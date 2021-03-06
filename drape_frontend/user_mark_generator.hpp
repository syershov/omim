#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/pointers.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace df
{
using MarksIDGroups = std::map<MarkGroupID, drape_ptr<MarkIDCollection>>;
using MarksIndex = std::map<TileKey, drape_ptr<MarksIDGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = function<void(TUserMarksRenderData && renderData)>;

  UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(drape_ptr<UserLinesRenderCollection> && lines);
  void SetRemovedUserMarks(drape_ptr<MarkIDCollection> && ids);
  void SetCreatedUserMarks(drape_ptr<MarkIDCollection> && ids);

  void SetGroup(MarkGroupID groupId, drape_ptr<MarkIDCollection> && ids);
  void RemoveGroup(MarkGroupID groupId);
  void SetGroupVisibility(MarkGroupID groupId, bool isVisible);

  void GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures);

private:
  void UpdateIndex(MarkGroupID groupId);

  ref_ptr<MarkIDCollection> GetIdCollection(TileKey const & tileKey, MarkGroupID groupId);
  void CleanIndex();

  std::unordered_set<MarkGroupID> m_groupsVisibility;
  MarksIDGroups m_groups;

  UserMarksRenderCollection m_marks;
  UserLinesRenderCollection m_lines;

  MarksIndex m_index;

  TFlushFn m_flushFn;
};
}  // namespace df
