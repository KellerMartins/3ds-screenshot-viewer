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

const std::string app_folder_path = "/3ds/Screenshot-Viewer/";
const std::string setings_path = app_folder_path + "settings.json";

std::string tags_path = app_folder_path + "tags.json";
std::string screenshots_path = "/luma/screenshots";

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

    if (!std::filesystem::exists(app_folder_path)) {
        std::filesystem::create_directories(app_folder_path);
    }

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