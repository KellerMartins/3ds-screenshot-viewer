#ifndef TAGS_HPP_
#define TAGS_HPP_

#include <3ds.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace tags {
struct Tag {
    std::string name;
    u32 color;
};

using tag_ptr = const Tag*;

void Load();
void Save();

tag_ptr Get(size_t index);
int GetTagIndex(tag_ptr tag);
std::vector<tag_ptr>& GetScreenshotTags(std::string screenshot_name);
std::set<tag_ptr> GetScreenshotsTags(std::set<std::string> screenshot_names);
const std::set<tags::tag_ptr> GetTagsFilter();
const std::set<tags::tag_ptr> GetHiddenTags();

size_t Count();
bool WasModified();

void ChangeScreenshotsTags(std::set<std::string> screenshot_names, std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags);
void RemoveScreenshotsTags(std::set<std::string> screenshot_names);
void ChangeTagsFilter(std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags);
void ChangeHiddenTags(std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags);

tag_ptr AddTag(Tag new_tag);
void ReplaceTag(tag_ptr tag, Tag new_tag);
void MoveTag(size_t src_idx, size_t dst_idx);
void DeleteTag(tag_ptr tag);
}  // namespace tags

#endif  // TAGS_HPP_
