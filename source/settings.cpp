#include "settings.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define TOML_EXCEPTIONS 0
#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.hpp>

namespace settings {

const std::string app_folder_path = "/3ds/Screenshot-Viewer/";
const std::string setings_path = app_folder_path + "settings.toml";

std::string tags_path = app_folder_path + "tags.toml";
std::string screenshots_path = "/luma/screenshots";

int extra_stereo_offset = 7;
bool show_console = false;

void Save() {
    std::ofstream f(setings_path);
    f << "screenshots_path = \"" << screenshots_path << "\"\n"
      << "tags_path = \"" << tags_path << "\"\n"
      << "extra_stereo_offset = " << extra_stereo_offset << "\n"
      << "show_console = " << (show_console ? "true" : "false") << "\n";
    f.close();
}

void Load() {
    if (std::filesystem::exists(setings_path)) {
        toml::parse_result result = toml::parse_file(setings_path);
        if (!result) return;

        toml::table data = std::move(result).table();

        screenshots_path = data["screenshots_path"].value_or(screenshots_path);
        std::cout << screenshots_path << "\n";
        tags_path = data["tags_path"].value_or(tags_path);
        show_console = data["show_console"].value_or(show_console);

    } else {
        Save();
    }
}

const std::string ScreenshotsPath() { return screenshots_path; }
const std::string TagsPath() { return tags_path; }
const int ExtraStereoOffset() { return extra_stereo_offset; }
const bool ShowConsole() { return show_console; }
}  // namespace settings
