#include "screenshots.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "loadbmp.hpp"
#include "settings.hpp"
#include "tags.hpp"
#include "threads/screenshot_thread.hpp"
#include "threads/thumbnail_thread.hpp"
#include "ui.hpp"

namespace screenshots {

enum SCRS_TYPE {
    TOP = 0,
    TOP_RIGHT = 1,
    BOTTOM = 2,
};

std::string suffixes[] = {"_top.bmp", "_top_right.bmp", "_bot.bmp"};

std::vector<mutable_info_ptr> screenshots;

std::vector<mutable_info_ptr> screenshots_shown;
std::vector<mutable_info_ptr> screenshots_hidden;
ScreenshotOrder screenshot_order = kTags;

threads::ScreenshotThread *screenshotThread;
threads::ThumbnailThread *thumbnailThread;

void SearchScreenshots() {
    auto files = std::vector<std::filesystem::path>();

    for (const auto &entry : std::filesystem::directory_iterator(settings::ScreenshotsPath())) files.push_back(entry.path());

    std::sort(files.begin(), files.end());

    for (size_t i = 0; i < files.size(); i++) {
        SCRS_TYPE type;
        std::string name;

        std::string filename = files[i].filename().string();
        for (size_t s = 0; s < sizeof(suffixes) / sizeof(*suffixes); s++) {
            if (filename.ends_with(suffixes[s])) {
                type = (SCRS_TYPE)s;
                name = filename.substr(0, filename.size() - suffixes[s].size());

                if (screenshots.size() == 0 || screenshots.back()->name != name) {
                    screenshots.push_back(std::shared_ptr<ScreenshotInfo>(new ScreenshotInfo(name, tags::GetScreenshotTags(name))));
                }

                auto scrs = screenshots.back();
                switch (type) {
                    case TOP:
                        scrs->path_top = files[i];
                        break;
                    case TOP_RIGHT:
                        scrs->path_top_right = files[i];
                        break;
                    case BOTTOM:
                        scrs->path_bottom = files[i];
                        break;
                }

                break;
            }
        }
    }
}

void UpdateOrder() {
    std::list<mutable_info_ptr> filtered_screenshots;
    screenshots_hidden.clear();

    for (size_t i = 0; i < screenshots.size(); i++) {
        if ((tags::GetTagsFilter().size() == 0 || screenshots[i]->has_all_tag(tags::GetTagsFilter())) && !screenshots[i]->has_any_tag(tags::GetHiddenTags())) {
            filtered_screenshots.push_back(screenshots[i]);
        } else {
            screenshots_hidden.push_back(screenshots[i]);
        }
    }

    if (thumbnailThread) thumbnailThread->Stop();
    if (screenshotThread) screenshotThread->Stop();

    screenshots_shown.clear();
    screenshots_shown.reserve(filtered_screenshots.size());
    switch (screenshot_order) {
        case kNewer:
            std::reverse(filtered_screenshots.begin(), filtered_screenshots.end());
            // pass through
        case kOlder:
            std::copy(filtered_screenshots.begin(), filtered_screenshots.end(), std::back_inserter(screenshots_shown));
            break;
        case kTags:
        case kTagsNewer:
            std::map<tags::tag_ptr, std::list<mutable_info_ptr>> index_groups = {{nullptr, {}}};
            std::set<tags::tag_ptr> processed_tags;
            std::vector<tags::tag_ptr> tags_order;

            if (screenshot_order == kTagsNewer) {
                std::reverse(filtered_screenshots.begin(), filtered_screenshots.end());
            }

            for (auto it = filtered_screenshots.begin(); it != filtered_screenshots.end(); ++it) {
                auto screenshot_tags = (*it)->tags;
                if (screenshot_tags.size() == 0) {
                    index_groups[nullptr].push_back(*it);
                    continue;
                }

                if (!processed_tags.contains(screenshot_tags[0])) {
                    index_groups[screenshot_tags[0]] = {*it};
                    processed_tags.insert(screenshot_tags[0]);
                    tags_order.push_back(screenshot_tags[0]);
                } else {
                    index_groups[screenshot_tags[0]].push_back(*it);
                }
            }
            tags_order.push_back(nullptr);

            if (screenshot_order == kTags) {
                for (size_t i = 0; i < tags::Count(); i++) {
                    auto tag = tags::Get(i);
                    if (processed_tags.contains(tag)) {
                        auto group = index_groups[tag];
                        group.sort([](mutable_info_ptr s1, mutable_info_ptr s2) {
                            auto s1_tags = s1->tags;
                            auto s2_tags = s2->tags;
                            if (s1_tags.size() != s2_tags.size()) {
                                // Order by number of tags (less first)
                                return s1_tags.size() < s2_tags.size();
                            } else {
                                // Order by tag order (less first)
                                int s1_sum = 0;
                                int s2_sum = 0;
                                for (auto tag : s1_tags) s1_sum += tags::GetTagIndex(tag);
                                for (auto tag : s2_tags) s2_sum += tags::GetTagIndex(tag);

                                return s1_sum < s2_sum;
                            }
                        });

                        screenshots_shown.insert(screenshots_shown.end(), group.begin(), group.end());
                    }
                }

                screenshots_shown.insert(screenshots_shown.end(), index_groups[nullptr].begin(), index_groups[nullptr].end());
            } else {
                for (auto tag : tags_order) {
                    auto group = index_groups[tag];
                    screenshots_shown.insert(screenshots_shown.end(), group.begin(), group.end());
                }
            }
            break;
    }

    if (screenshotThread) screenshotThread->Start();
    if (thumbnailThread) thumbnailThread->Start(screenshots_shown.begin(), screenshots_shown.end());
}

void Init() {
    SearchScreenshots();
    UpdateOrder();

    screenshotThread = new threads::ScreenshotThread();
    thumbnailThread = new threads::ThumbnailThread(screenshots_shown.begin(), screenshots_shown.end());
}

void Exit() { delete screenshotThread; }

void Load(info_ptr info, void (*callback)(screenshot_ptr)) {
    if (screenshotThread) screenshotThread->Load(info, callback);
}

size_t Count() { return screenshots_shown.size(); }
size_t NumLoadedThumbnails() {
    if (thumbnailThread) return thumbnailThread->NumLoadedThumbnails();
    return 0;
}
bool FoundScreenshots() { return screenshots.size() > 0; }

info_ptr GetInfo(std::size_t index) {
    if (index >= screenshots_shown.size()) return nullptr;

    if (thumbnailThread) thumbnailThread->SetCurrent(screenshots_shown.begin() + index);

    return screenshots_shown[index];
}

const ScreenshotOrder GetOrder() { return screenshot_order; }
void SetOrder(ScreenshotOrder order) {
    screenshot_order = order;
    UpdateOrder();
}

void Delete(std::set<std::string> screenshot_names) {
    std::vector<mutable_info_ptr> new_screenshots;
    std::vector<mutable_info_ptr> deleted_screenshots;
    for (size_t i = 0; i < screenshots.size(); i++) {
        if (screenshot_names.contains(screenshots[i]->name)) {
            deleted_screenshots.push_back(std::move(screenshots[i]));
        } else {
            new_screenshots.push_back(std::move(screenshots[i]));
        }
    }

    if (thumbnailThread) thumbnailThread->Stop();
    if (screenshotThread) screenshotThread->Stop();

    screenshots = std::move(new_screenshots);

    for (auto &screenshot : deleted_screenshots) {
        try {
            if (screenshot->path_top != "") std::filesystem::remove(screenshot->path_top);
            if (screenshot->path_top_right != "") std::filesystem::remove(screenshot->path_top_right);
            if (screenshot->path_bottom != "") std::filesystem::remove(screenshot->path_bottom);
        } catch (const std::filesystem::filesystem_error &err) {
            std::cout << "Error deleting screenshots: " << err.what() << '\n';
        }
    }

    tags::RemoveScreenshotsTags(screenshot_names);
    UpdateOrder();
}

bool ScreenshotInfo::has_any_tag(std::set<tags::tag_ptr> tags) {
    for (auto &tag : this->tags) {
        if (tags.contains(tag)) return true;
    }
    return false;
}

bool ScreenshotInfo::has_all_tag(std::set<tags::tag_ptr> tags) {
    for (auto &tag : tags) {
        if (std::find(this->tags.begin(), this->tags.end(), tag) == this->tags.end()) return false;
    }
    return true;
}

ScreenshotInfo::ScreenshotInfo(std::string name, const std::vector<tags::tag_ptr> &tags) : name(name), tags(tags), has_thumbnail(false) {}
}  // namespace screenshots
