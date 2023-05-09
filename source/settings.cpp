#include "settings.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace settings {

std::string screenshots_path = "/luma/screenshots";
std::string tags_path = "./tags.json";

bool show_console = false;

void save() {
    std::ofstream f(setings_path);
    f << "{\n"
      << "\"screenshots_path\": \"" << screenshots_path << "\",\n"
      << "\"tags_path\": \"" << tags_path << "\",\n"
      << "\"show_console\": " << (show_console ? "true" : "false") << "\n"
      << "}\n";
    f.close();
}

void load() {
    json settings;

    if (std::filesystem::exists(setings_path)) {
        std::ifstream f(setings_path);
        settings = json::parse(f);
        f.close();
    } else {
        save();
    }
}

const std::string get_screenshots_path() { return screenshots_path; }
const std::string get_tags_path() { return tags_path; }
const bool get_show_console() { return show_console; }
}  // namespace settings