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

void load();
void save();
std::vector<int>& get_tag_ids(std::string screenshot_name);
}  // namespace tags

#endif
