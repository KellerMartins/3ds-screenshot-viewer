#ifndef TAGS_HPP_
#define TAGS_HPP_

#include <3ds.h>

#include <string>
#include <vector>

namespace tags {
struct Tag {
    std::string name;
    u32 color;
    u32 text_color;
};

void Load();
void Save();
std::vector<int>& GetScreenshotTagIds(std::string screenshot_name);
}  // namespace tags

#endif  // TAGS_HPP_
