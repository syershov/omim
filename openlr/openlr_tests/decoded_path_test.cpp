#include "testing/testing.hpp"

#include "openlr/decoded_path.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"

#include "base/checked_cast.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include "3party/pugixml/src/pugixml.hpp"
#include "3party/pugixml/src/utils.hpp"

using namespace generator::tests_support;
using namespace platform::tests_support;
using namespace platform;
using namespace std;

namespace
{
string const kTestDir = "openlr_decoded_path_test";
string const kTestMwm = "test";

double RoughUpToFive(double d)
{
  stringstream s;
  s << setprecision(5) << fixed;
  s << d;
  s >> d;
  return d;
}

m2::PointD RoughPoint(m2::PointD const & p) { return {RoughUpToFive(p.x), RoughUpToFive(p.y)}; }

routing::Junction RoughJunction(routing::Junction const & j)
{
  return routing::Junction(RoughPoint(j.GetPoint()), j.GetAltitude());
}

routing::Edge RoughEdgeJunctions(routing::Edge const & e)
{
  return routing::Edge::MakeReal(e.GetFeatureId(), e.IsForward(), e.GetSegId(),
                                 RoughJunction(e.GetStartJunction()),
                                 RoughJunction(e.GetEndJunction()));
}

void RoughJunctionsInPath(openlr::Path & p)
{
  for (auto & e : p)
    e = RoughEdgeJunctions(e);
}

void TestSerializeDeserialize(openlr::Path const & path, DataSource const & index)
{
  pugi::xml_document doc;
  openlr::PathToXML(path, doc);

  openlr::Path restoredPath;
  openlr::PathFromXML(doc, index, restoredPath);

  // Fix MercatorBounds::From/ToLatLon floating point error
  // for we could use TEST_EQUAL on result.
  RoughJunctionsInPath(restoredPath);

  TEST_EQUAL(path, restoredPath, ());
}

openlr::Path MakePath(FeatureType const & road, bool const forward)
{
  CHECK_EQUAL(road.GetFeatureType(), feature::GEOM_LINE, ());
  CHECK_GREATER(road.GetPointsCount(), 0, ());
  openlr::Path path;

  size_t const maxPointIndex = road.GetPointsCount() - 1;
  for (size_t i = 0; i < maxPointIndex; ++i)
  {
    size_t current{};
    size_t next{};
    if (forward)
    {
      current = i;
      next = i + 1;
    }
    else
    {
      current = maxPointIndex - i;
      next = current - 1;
    }

    auto const from = road.GetPoint(current);
    auto const to = road.GetPoint(next);
    path.push_back(routing::Edge::MakeReal(
        road.GetID(), forward,
        base::checked_cast<uint32_t>(current - static_cast<size_t>(!forward)) /* segId */,
        routing::Junction(from, 0 /* altitude */), routing::Junction(to, 0 /* altitude */)));
  }

  RoughJunctionsInPath(path);

  return path;
}

template <typename Func>
void WithRoad(vector<m2::PointD> const & points, Func && fn)
{
  classificator::Load();
  auto & platform = GetPlatform();

  auto const mwmPath = my::JoinPath(platform.WritableDir(), kTestDir);

  LocalCountryFile country(mwmPath, CountryFile(kTestMwm), 0 /* version */);
  ScopedDir testScopedDir(kTestDir);
  ScopedFile testScopedMwm(my::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION),
                           ScopedFile::Mode::Create);

  {
    TestMwmBuilder builder(country, feature::DataHeader::country);
    builder.Add(TestRoad(points, "Interstate 60", "en"));
  }

  DataSource index;
  auto const regResult = index.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  MwmSet::MwmHandle mwmHandle = index.GetMwmHandleById(regResult.first);
  TEST(mwmHandle.IsAlive(), ());

  DataSource::FeaturesLoaderGuard const guard(index, regResult.first);
  FeatureType road;
  TEST(guard.GetFeatureByIndex(0, road), ());
  road.ParseEverything();

  fn(index, road);
}

UNIT_TEST(MakePath_Test)
{
  std::vector<m2::PointD> const points{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
  WithRoad(points, [&points](DataSource const & index, FeatureType & road) {
    auto const & id = road.GetID();
    {
      openlr::Path const expected{
          routing::Edge::MakeReal(id, true /* forward */, 0 /* segId*/,
                                  {points[0], 0 /* altitude */}, {points[1], 0 /* altitude */}),
          routing::Edge::MakeReal(id, true /* forward */, 1 /* segId*/,
                                  {points[1], 0 /* altitude */}, {points[2], 0 /* altitude */}),
          routing::Edge::MakeReal(id, true /* forward */, 2 /* segId*/,
                                  {points[2], 0 /* altitude */}, {points[3], 0 /* altitude */})};
      auto const path = MakePath(road, true /* forward */);
      TEST_EQUAL(path, expected, ());
    }
    {
      openlr::Path const expected{
          routing::Edge::MakeReal(id, false /* forward */, 2 /* segId*/,
                                  {points[3], 0 /* altitude */}, {points[2], 0 /* altitude */}),
          routing::Edge::MakeReal(id, false /* forward */, 1 /* segId*/,
                                  {points[2], 0 /* altitude */}, {points[1], 0 /* altitude */}),
          routing::Edge::MakeReal(id, false /* forward */, 0 /* segId*/,
                                  {points[1], 0 /* altitude */}, {points[0], 0 /* altitude */})};
      {
        auto const path = MakePath(road, false /* forward */);
        TEST_EQUAL(path, expected, ());
      }
    }
  });
}

UNIT_TEST(PathSerializeDeserialize_Test)
{
  WithRoad({{0, 0}, {0, 1}, {1, 0}, {1, 1}}, [](DataSource const & index, FeatureType & road) {
    {
      auto const path = MakePath(road, true /* forward */);
      TestSerializeDeserialize(path, index);
    }
    {
      auto const path = MakePath(road, false /* forward */);
      TestSerializeDeserialize(path, index);
    }
  });
}

UNIT_TEST(GetPoints_Test)
{
  vector<m2::PointD> const points{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
  WithRoad(points, [&points](DataSource const &, FeatureType & road) {
    {
      auto path = MakePath(road, true /* forward */);
      // RoughJunctionsInPath(path);
      TEST_EQUAL(GetPoints(path), points, ());
    }
    {
      auto path = MakePath(road, false /* forward */);
      // RoughJunctionsInPath(path);
      auto reversed = points;
      reverse(begin(reversed), end(reversed));
      TEST_EQUAL(GetPoints(path), reversed, ());
    }
  });
}
}  // namespace
