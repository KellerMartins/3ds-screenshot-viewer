#include "tags.hpp"

#include <3ds.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#define TOML_EXCEPTIONS 0
#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.hpp>

#include "settings.hpp"

namespace tags {

std::vector<std::shared_ptr<Tag>> tags;
std::map<std::string, std::vector<std::shared_ptr<const Tag>>> screenshot_tags;

std::set<tags::tag_ptr> tags_filter;
std::set<tags::tag_ptr> hidden_tags;

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

int GetTagIndex(std::shared_ptr<const Tag> tag) {
    for (size_t i = 0; i < tags.size(); i++) {
        if (tag == tags[i]) {
            return i;
        }
    }
    return -1;
}

void Save() {
    std::ofstream f(settings::TagsPath());
    f << "tags = [\n";
    for (size_t i = 0; i < tags.size(); i++) {
        f << "  { "
          << "name = \"" << tags[i]->name << "\", "
          << "color = \"" << color_to_hex_string(tags[i]->color) << "\"";
        f << " },\n";
    }
    f << "]\n\n";

    f << "tags_filter = [";
    size_t i = 0;
    for (auto it = tags_filter.begin(); it != tags_filter.end(); it++) {
        f << GetTagIndex(*it) << (i == tags_filter.size() - 1 ? "" : ", ");
        i++;
    }
    f << "]\n";

    f << "hidden_tags = [";
    i = 0;
    for (auto it = hidden_tags.begin(); it != hidden_tags.end(); it++) {
        f << GetTagIndex(*it) << (i == hidden_tags.size() - 1 ? "" : ", ");
        i++;
    }
    f << "]\n\n";

    f << "[screenshot_tags]\n";
    for (auto const& kv : screenshot_tags) {
        f << "  \"" << kv.first << "\" = [";
        for (size_t i = 0; i < kv.second.size(); i++) {
            f << GetTagIndex(kv.second[i]) << (i == kv.second.size() - 1 ? "" : ", ");
        }
        f << "]\n";
    }

    f.close();
}

void Load() {
    tags.clear();
    screenshot_tags.clear();

    const std::string tags_path = settings::TagsPath();
    if (std::filesystem::exists(tags_path)) {
        toml::parse_result result = toml::parse_file(tags_path);
        if (!result) {
            std::cerr << "Parsing failed:\n" << result.error() << "\n";
            return;
        }

        toml::table data = std::move(result).table();

        if (toml::array* arr = data["tags"].as_array()) {
            arr->for_each([](auto&& el) {
                if constexpr (toml::is_table<decltype(el)>) {
                    if (el["name"].is_string() && el["color"].is_string()) {
                        tags.push_back(std::shared_ptr<Tag>(new Tag({
                            el["name"].as_string()->get(),
                            color_from_hex_string(el["color"].as_string()->get()),
                        })));
                    }
                }
            });
        }

        if (toml::array* arr = data["tags_filter"].as_array()) {
            arr->for_each([](auto&& el) {
                if constexpr (toml::is_number<decltype(el)>) {
                    tags_filter.insert(tags[el.get()]);
                }
            });
        }

        if (toml::array* arr = data["hidden_tags"].as_array()) {
            arr->for_each([](auto&& el) {
                if constexpr (toml::is_number<decltype(el)>) {
                    hidden_tags.insert(tags[el.get()]);
                }
            });
        }

        if (toml::table* tbl = data["screenshot_tags"].as_table()) {
            for (auto&& [key, value] : *tbl) {
                std::string screenshot_name = std::string(key.str());
                screenshot_tags[screenshot_name] = {};

                if (toml::array* ids_arr = value.as_array()) {
                    ids_arr->for_each([screenshot_name](auto&& el) {
                        if constexpr (toml::is_integer<decltype(el)>) {
                            if (el.get() >= 0 && el.get() < tags.size()) {
                                screenshot_tags[screenshot_name].push_back(tags[el.get()]);
                            }
                        }
                    });
                }
            }
        }
    } else {
        Save();
    }
}

std::vector<std::shared_ptr<const Tag>>& GetScreenshotTags(std::string screenshot_name) {
    if (!screenshot_tags.contains(screenshot_name)) {
        screenshot_tags[screenshot_name] = {};
    }

    return screenshot_tags[screenshot_name];
}

std::set<std::shared_ptr<const Tag>> GetScreenshotsTags(std::set<std::string> screenshot_names) {
    std::set<std::shared_ptr<const Tag>> tags_set;
    for (const std::string& name : screenshot_names) {
        if (screenshot_tags.contains(name)) {
            for (auto tag : screenshot_tags[name]) {
                tags_set.insert(tag);
            }
        }
    }

    return tags_set;
}

std::shared_ptr<const Tag> Get(size_t index) { return tags[index]; }

size_t Count() { return tags.size(); }

void SetScreenshotsTags(std::set<std::string> screenshot_names, std::set<std::shared_ptr<const Tag>> tags) {
    for (const std::string& name : screenshot_names) {
        screenshot_tags[name] = {};

        for (auto tag : tags) {
            screenshot_tags[name].push_back(tag);
        }
    }
}

std::shared_ptr<const Tag> AddTag(Tag new_tag) {
    auto ptr = std::shared_ptr<Tag>(new Tag(new_tag));
    tags.push_back(ptr);
    return ptr;
}

void ReplaceTag(std::shared_ptr<const Tag> tag, Tag new_tag) {
    int idx = GetTagIndex(tag);
    if (idx >= 0) {
        *tags[idx] = new_tag;
    }
}
void MoveTag(size_t src_idx, size_t dst_idx) {
    if (src_idx == dst_idx) return;

    if (src_idx >= tags.size() || dst_idx >= tags.size()) return;

    auto t = tags[src_idx];
    tags.erase(tags.begin() + src_idx);
    tags.insert(tags.begin() + dst_idx, t);
}

void DeleteTag(std::shared_ptr<const Tag> tag) {
    int idx = GetTagIndex(tag);
    if (idx >= 0) {
        tags.erase(tags.begin() + idx);

        for (auto& kv : screenshot_tags) {
            kv.second.erase(std::remove_if(kv.second.begin(), kv.second.end(), [tag](auto t) { return t == tag; }), kv.second.end());
        }
    }
}

const std::set<tags::tag_ptr> GetTagsFilter() { return tags_filter; }

const std::set<tags::tag_ptr> GetHiddenTags() { return hidden_tags; }

void SetTagsFilter(std::set<tags::tag_ptr> tags) { tags_filter = tags; }

void SetHiddenTags(std::set<tags::tag_ptr> tags) { hidden_tags = tags; }
}  // namespace tags
