#ifndef TAGS_HPP_
#define TAGS_HPP_

#include <3ds.h>

#include <set>
#include <string>
#include <vector>

namespace tags {
struct Tag {
    std::string name;
    u32 color;
};

using TagId = unsigned int;

void Load();
void Save();
std::vector<TagId>& GetScreenshotTagIds(std::string screenshot_name);
std::set<TagId> GetScreenshotsTagIds(std::set<std::string> screenshot_names);
Tag* Get(TagId id);
size_t Size();

void SetScreenshotsTagIds(std::set<std::string> screenshot_names, std::set<TagId> tags);
}  // namespace tags

#endif  // TAGS_HPP_
