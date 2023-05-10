#include "settings.hpp"

#include <filesystem>
#include <fstream>
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

void Save() {
    std::ofstream f(setings_path);
    f << "{\n"
      << "\"screenshots_path\": \"" << screenshots_path << "\",\n"
      << "\"tags_path\": \"" << tags_path << "\",\n"
      << "\"show_console\": " << (show_console ? "true" : "false") << "\n"
      << "}\n";
    f.close();
}

void Load() {
    json settings;

    if (!std::filesystem::exists(app_folder_path)) {
        std::filesystem::create_directories(app_folder_path);
    }

    if (std::filesystem::exists(setings_path)) {
        std::ifstream f(setings_path);
        settings = json::parse(f);
        f.close();

        screenshots_path = settings["screenshots_path"];
        tags_path = settings["tags_path"];
        show_console = settings["show_console"];
    } else {
        Save();
    }
}

const std::string ScreenshotsPath() { return screenshots_path; }
const std::string TagsPath() { return tags_path; }
const bool ShowConsole() { return show_console; }
}  // namespace settings
