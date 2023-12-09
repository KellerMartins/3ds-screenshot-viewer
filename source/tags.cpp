#include "tags.hpp"

#include <3ds.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#define TOML_EXCEPTIONS 0
#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.hpp>

#include "screenshots.hpp"
#include "settings.hpp"

namespace tags {

std::vector<Tag*> tags;
std::map<std::string, std::vector<const Tag*>> screenshot_tags;

std::set<tags::tag_ptr> tags_filter;
std::set<tags::tag_ptr> hidden_tags;

bool modified = false;

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

int GetTagIndex(tag_ptr tag) {
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
                        tags.push_back(new Tag({
                            el["name"].as_string()->get(),
                            color_from_hex_string(el["color"].as_string()->get()),
                        }));
                    }
                }
            });
        }

        if (toml::array* arr = data["tags_filter"].as_array()) {
            arr->for_each([](auto&& el) {
                if constexpr (toml::is_number<decltype(el)>) {
                    if (el.get() >= 0 && el.get() < tags.size()) {
                        tags_filter.insert(tags[el.get()]);
                    }
                }
            });
        }

        if (toml::array* arr = data["hidden_tags"].as_array()) {
            arr->for_each([](auto&& el) {
                if constexpr (toml::is_number<decltype(el)>) {
                    if (el.get() >= 0 && el.get() < tags.size()) {
                        hidden_tags.insert(tags[el.get()]);
                    }
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
        modified = true;
    }
}

std::vector<tag_ptr>& GetScreenshotTags(std::string screenshot_name) {
    if (!screenshot_tags.contains(screenshot_name)) {
        screenshot_tags[screenshot_name] = {};
    }

    return screenshot_tags[screenshot_name];
}

std::set<tag_ptr> GetScreenshotsTags(std::set<std::string> screenshot_names) {
    std::set<tag_ptr> tags_set;
    for (const std::string& name : screenshot_names) {
        if (screenshot_tags.contains(name)) {
            for (auto tag : screenshot_tags[name]) {
                tags_set.insert(tag);
            }
        }
    }

    return tags_set;
}

tag_ptr Get(size_t index) { return tags[index]; }

size_t Count() { return tags.size(); }

void ChangeScreenshotsTags(std::set<std::string> screenshot_names, std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags) {
    for (const std::string& name : screenshot_names) {
        auto new_tags = std::set<tag_ptr>(screenshot_tags[name].begin(), screenshot_tags[name].end());
        for (auto tag : added_tags) {
            new_tags.insert(tag);
        }
        for (auto tag : removed_tags) {
            new_tags.erase(tag);
        }

        screenshot_tags[name] = {};
        for (auto tag : tags) {  // Iterates through all tags so the inserted order matches the tags order
            if (new_tags.contains(tag)) {
                screenshot_tags[name].push_back(tag);
            }
        }
    }

    modified = true;
    screenshots::UpdateOrder();
}

void RemoveScreenshotsTags(std::set<std::string> screenshot_names) {
    for (const std::string& name : screenshot_names) {
        if (screenshot_tags.contains(name)) {
            screenshot_tags.erase(name);
        }
    }
    modified = true;
}

tag_ptr AddTag(Tag new_tag) {
    auto ptr = new Tag(new_tag);
    tags.push_back(ptr);

    modified = true;

    return ptr;
}

void ReplaceTag(tag_ptr tag, Tag new_tag) {
    int idx = GetTagIndex(tag);
    if (idx >= 0) {
        *tags[idx] = new_tag;

        modified = true;
    }
}

void MoveTag(size_t src_idx, size_t dst_idx) {
    if (src_idx == dst_idx) return;

    if (src_idx >= tags.size() || dst_idx >= tags.size()) return;

    auto t = tags[src_idx];
    tags.erase(tags.begin() + src_idx);
    tags.insert(tags.begin() + dst_idx, t);

    modified = true;
    screenshots::UpdateOrder();
}

void DeleteTag(tag_ptr tag) {
    int idx = GetTagIndex(tag);
    if (idx >= 0) {
        delete tags[idx];
        tags.erase(tags.begin() + idx);

        for (auto& kv : screenshot_tags) {
            kv.second.erase(std::remove_if(kv.second.begin(), kv.second.end(), [tag](auto t) { return t == tag; }), kv.second.end());
        }

        if (tags_filter.contains(tag)) {
            tags_filter.erase(tag);
        }

        if (hidden_tags.contains(tag)) {
            hidden_tags.erase(tag);
        }

        modified = true;
        screenshots::UpdateOrder();
    }
}

const std::set<tags::tag_ptr> GetTagsFilter() { return tags_filter; }

const std::set<tags::tag_ptr> GetHiddenTags() { return hidden_tags; }

void ChangeTagsFilter(std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags) {
    for (auto tag : added_tags) tags_filter.insert(tag);
    for (auto tag : removed_tags) tags_filter.erase(tag);

    modified = true;
    screenshots::UpdateOrder();
}

void ChangeHiddenTags(std::set<tag_ptr> added_tags, std::set<tag_ptr> removed_tags) {
    for (auto tag : added_tags) hidden_tags.insert(tag);
    for (auto tag : removed_tags) hidden_tags.erase(tag);

    modified = true;
    screenshots::UpdateOrder();
}

bool WasModified() { return modified; }

}  // namespace tags
