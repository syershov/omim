#pragma once

#include "search/cities_boundaries_table.hpp"

#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>

class DataSourceBase;

namespace search
{
class VillagesCache;

struct LocalityItem
{
  using Boundaries = CitiesBoundariesTable::Boundaries;

  LocalityItem(StringUtf8Multilang const & names, m2::PointD const & center,
               Boundaries const & boundaries, uint64_t population, FeatureID const & id);

  bool GetName(int8_t lang, string & name) const { return m_names.GetString(lang, name); }

  bool GetSpecifiedOrDefaultName(int8_t lang, string & name) const
  {
    return GetName(lang, name) || GetName(StringUtf8Multilang::kDefaultCode, name);
  }

  StringUtf8Multilang m_names;
  m2::PointD m_center;
  Boundaries m_boundaries;
  uint64_t m_population;
  FeatureID m_id;
};

string DebugPrint(LocalityItem const & item);

class LocalitySelector
{
public:
  LocalitySelector(m2::PointD const & p);

  void operator()(LocalityItem const & item);

  template <typename Fn>
  bool WithBestLocality(Fn && fn) const
  {
    if (!m_locality)
      return false;
    fn(*m_locality);
    return true;
  }

private:
  m2::PointD const m_p;

  bool m_inside = false;
  double m_score = std::numeric_limits<double>::max();
  LocalityItem const * m_locality = nullptr;
};

class LocalityFinder
{
public:
  class Holder
  {
  public:
    Holder(double radiusMeters);

    bool IsCovered(m2::RectD const & rect) const;
    void SetCovered(m2::PointD const & p);

    void Add(LocalityItem const & item);
    void ForEachInVicinity(m2::RectD const & rect, LocalitySelector & selector) const;

    m2::RectD GetRect(m2::PointD const & p) const;
    m2::RectD GetDRect(m2::PointD const & p) const;

    void Clear();

  private:
    double const m_radiusMeters;
    m4::Tree<bool> m_coverage;
    m4::Tree<LocalityItem> m_localities;

    DISALLOW_COPY_AND_MOVE(Holder);
  };

  LocalityFinder(DataSourceBase const & index, CitiesBoundariesTable const & boundaries,
                 VillagesCache & villagesCache);

  template <typename Fn>
  bool GetLocality(m2::PointD const & p, Fn && fn)
  {
    m2::RectD const crect = m_cities.GetRect(p);
    m2::RectD const vrect = m_villages.GetRect(p);

    LoadVicinity(p, !m_cities.IsCovered(crect) /* loadCities */,
                 !m_villages.IsCovered(vrect) /* loadVillages */);

    LocalitySelector selector(p);
    m_cities.ForEachInVicinity(crect, selector);
    m_villages.ForEachInVicinity(vrect, selector);

    return selector.WithBestLocality(std::forward<Fn>(fn));
  }

  void ClearCache();

private:
  void LoadVicinity(m2::PointD const & p, bool loadCities, bool loadVillages);
  void UpdateMaps();

  DataSourceBase const & m_index;
  CitiesBoundariesTable const & m_boundariesTable;
  VillagesCache & m_villagesCache;

  Holder m_cities;
  Holder m_villages;

  m4::Tree<MwmSet::MwmId> m_maps;
  MwmSet::MwmId m_worldId;
  bool m_mapsLoaded;

  std::unique_ptr<RankTable> m_ranks;

  std::map<MwmSet::MwmId, std::unordered_set<uint32_t>> m_loadedIds;
};
}  // namespace search
