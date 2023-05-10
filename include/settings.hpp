#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <string>

namespace settings {
void save();
void load();

const std::string get_screenshots_path();
const std::string get_tags_path();
const bool get_show_console();
}  // namespace settings

#endif
