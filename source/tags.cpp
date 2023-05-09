#include "tags.hpp"

#include <3ds.h>

#include <filesystem>
#include <fstream>
#include <iostream>
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

void save() {
    json tags_json = json::array();
    for (Tag& tag : tags) {
        tags_json.push_back(json({{"name", tag.name}, {"color", tag.color}, {"text_color", tag.text_color}}));
    }

    json colors_json = json::array();
    for (u32& color : tag_colors) {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << color;
        colors_json.push_back(stream.str());
    }

    std::ofstream f(settings::get_tags_path());
    f << "{\n"
      << "\"tags\": " << tags_json.dump() << ",\n"
      << "\"screenshot_tags\": " << json(screenshot_tags).dump() << ",\n"
      << "\"colors\": " << colors_json.dump() << "\n"
      << "}\n";
    f.close();
}

void load() {
    json tags_data;
    const std::string tags_path = settings::get_tags_path();
    if (std::filesystem::exists(tags_path)) {
        std::ifstream f(tags_path);
        tags_data = json::parse(f);
        f.close();
    } else {
        save();
    }
}

std::vector<int>& get_tag_ids(std::string screenshot_name) {
    if (!screenshot_tags.contains(screenshot_name)) {
        screenshot_tags[screenshot_name] = std::vector<int>();
    }

    return screenshot_tags[screenshot_name];
}
}  // namespace tags