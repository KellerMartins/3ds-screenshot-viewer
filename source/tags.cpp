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
    0x000000aa, 0x1d2b53aa, 0x7e2553aa, 0x008751aa, 0xab5236aa, 0x5f574faa, 0xc2c3c7aa, 0xfff1e8aa, 0xff004daa, 0xffa300aa, 0xffec27aa,
    0x00e436aa, 0x29adffaa, 0x83769caa, 0xff77a8aa, 0xffccaaaa, 0x291814aa, 0x111d35aa, 0x422136aa, 0x125359aa, 0x742f29aa, 0x49333baa,
    0xa28879aa, 0xf3ef7daa, 0xbe1250aa, 0xff6c24aa, 0xa8e72eaa, 0x00b543aa, 0x065ab5aa, 0x754665aa, 0xff6e59aa, 0xff9d81aa,
};

std::vector<Tag> tags;
std::map<std::string, std::vector<int>> screenshot_tags;

std::string color_to_hex_string(u32 x) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << x;
    return stream.str();
}

u32 color_from_hex_string(std::string x) {
    std::string color;
    if (x.size() == 8)
        color = x.substr(6, 2) + x.substr(4, 2) + x.substr(2, 2) + x.substr(0, 2);  // RGBA to ABGR
    else if (x.size() == 6)
        color = "ff" + x.substr(4, 2) + x.substr(2, 2) + x.substr(0, 2);  // RGB to ABGR
    else
        color = "ff000000";  // Invalid color, set to black

    return std::stoul(color, nullptr, 16);
}

void Save() {
    json tags_json = json::array();
    for (Tag& tag : tags) {
        tags_json.push_back(json({{"name", tag.name}, {"color", color_to_hex_string(tag.color)}}));
    }

    std::ofstream f(settings::TagsPath());
    f << "{\n"
      << "\"tags\": " << tags_json.dump() << ",\n"
      << "\"screenshot_tags\": " << json(screenshot_tags).dump() << ",\n"
      << "}\n";
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

std::vector<int>& GetScreenshotTagIds(std::string screenshot_name) {
    if (!screenshot_tags.contains(screenshot_name)) {
        screenshot_tags[screenshot_name] = std::vector<int>();
    }

    return screenshot_tags[screenshot_name];
}

std::vector<Tag>& GetTags() { return tags; }
}  // namespace tags
