#include "tags.hpp"

#include <3ds.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "settings.hpp"

using json = nlohmann::json;

namespace tags {
std::vector<u32> tag_colors = {
    0x000000ff, 0x1d2b53ff, 0x7e2553ff, 0x008751ff, 0xab5236ff, 0x5f574fff, 0xc2c3c7ff, 0xfff1e8ff, 0xff004dff, 0xffa300ff, 0xffec27ff,
    0x00e436ff, 0x29adffff, 0x83769cff, 0xff77a8ff, 0xffccaaff, 0x291814ff, 0x111d35ff, 0x422136ff, 0x125359ff, 0x742f29ff, 0x49333bff,
    0xa28879ff, 0xf3ef7dff, 0xbe1250ff, 0xff6c24ff, 0xa8e72eff, 0x00b543ff, 0x065ab5ff, 0x754665ff, 0xff6e59ff, 0xff9d81ff,
};

std::vector<Tag> tags;
std::map<std::string, std::vector<TagId>> screenshot_tags;

std::string color_to_hex_string(u32 x) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << x;
    std::string color = stream.str();

    return color.substr(6, 2) + color.substr(4, 2) + color.substr(2, 2);  // ABGR to RGB
}

u32 color_from_hex_string(std::string x) {
    std::string color;
    if (x.size() == 6)
        color = "ff" + x.substr(4, 2) + x.substr(2, 2) + x.substr(0, 2);  // RGB to ABGR
    else
        color = "ff000000";  // Invalid color, set to black

    return std::stoul(color, nullptr, 16);
}

void Save() {
    std::ofstream f(settings::TagsPath());
    f << "{\n"
      << "\"tags\": [\n";
    for (size_t i = 0; i < tags.size(); i++) {
        f << "  { "
          << "\"name\": \"" << tags[i].name << "\", "
          << "\"color\": \"" << color_to_hex_string(tags[i].color) << "\"";
        f << " }" << (i == tags.size() - 1 ? "\n" : ",\n");
    }
    f << "],\n";

    f << "\"screenshot_tags\": {\n";
    size_t i = 0;
    for (auto const& kv : screenshot_tags) {
        f << "  \"" << kv.first << "\": " << json(kv.second).dump();
        f << (i == screenshot_tags.size() - 1 ? "\n" : ",\n");
        i++;
    }
    f << "}}\n";
    f.close();
}

void Load() {
    json tags_data;

    const std::string tags_path = settings::TagsPath();
    if (std::filesystem::exists(tags_path)) {
        std::ifstream f(tags_path);
        tags_data = json::parse(f);

        screenshot_tags = tags_data["screenshot_tags"];

        for (auto& tag : tags_data["tags"]) {
            tags.push_back(Tag({
                tag["name"],
                color_from_hex_string(tag["color"]),
            }));
        }

        f.close();
    } else {
        Save();
    }
}

std::vector<TagId>& GetScreenshotTagIds(std::string screenshot_name) {
    if (!screenshot_tags.contains(screenshot_name)) {
        screenshot_tags[screenshot_name] = std::vector<TagId>();
    }

    return screenshot_tags[screenshot_name];
}

std::set<TagId> GetScreenshotsTagIds(std::set<std::string> screenshot_names) {
    std::set<TagId> tags_set;
    for (const std::string& name : screenshot_names) {
        if (screenshot_tags.contains(name)) {
            for (TagId& tag : screenshot_tags[name]) {
                tags_set.insert(tag);
            }
        }
    }

    return tags_set;
}

Tag* Get(TagId id) { return &tags[id]; }

size_t Size() { return tags.size(); }

void SetScreenshotsTagIds(std::set<std::string> screenshot_names, std::set<TagId> tags) {
    for (const std::string& name : screenshot_names) {
        screenshot_tags[name] = std::vector<TagId>();

        for (const TagId& tag : tags) {
            screenshot_tags[name].push_back(tag);
        }
    }

    Save();
}
}  // namespace tags
