#include <string>

namespace settings {
constexpr std::string setings_path = "./settings.json";

void save();
void load();

const std::string get_screenshots_path();
const std::string get_tags_path();
const bool get_show_console();
}  // namespace settings
