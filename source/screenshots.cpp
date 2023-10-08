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
#include "ui.hpp"

namespace screenshots {

enum SCRS_TYPE {
    TOP = 0,
    TOP_RIGHT = 1,
    BOTTOM = 2,
};

const int kStackSize = (4 * 1024);

std::string suffixes[] = {"_top.bmp", "_top_right.bmp", "_bot.bmp"};

std::vector<ScreenshotInfo> screenshots;

std::vector<size_t> screenshots_indexes;
std::vector<size_t> screenshots_hidden;
ScreenshotOrder screenshot_order = kTags;

std::atomic<bool> run_thread = true;
std::atomic<int> loaded_thumbs = 0;

std::atomic<size_t> last_screenshot_index = std::numeric_limits<size_t>::max();
std::atomic<size_t> loading_screenshot_index = std::numeric_limits<size_t>::max();
size_t next_screenshot_index = std::numeric_limits<size_t>::max();

// Use a screenshot for loading and another for returning to the callback
Screenshot *screenshot_buffer[2];
constexpr int num_buffers = (sizeof(screenshot_buffer) / sizeof(screenshot_buffer[0]));
std::atomic<int> current_buffer = 0;
int last_buffer = 0;

void (*load_screenshot_callback)(screenshot_ptr) = nullptr;

Thread thumbnailThread;
Thread loadScreenshotThread;
Handle loadScreenshotRequest;

C2D_Image CreateImage(u16 width, u16 height) {
    C3D_Tex *tex = new C3D_Tex;
    Tex3DS_SubTexture *subtex = new Tex3DS_SubTexture;

    subtex->width = static_cast<u16>(width);
    subtex->height = static_cast<u16>(height);

    u16 width_pow2 = std::bit_ceil(subtex->width);
    u16 height_pow2 = std::bit_ceil(subtex->height);

    subtex->top = 1.0f;
    subtex->left = 0.0f;
    subtex->right = subtex->width / static_cast<float>(width_pow2);
    subtex->bottom = 1.0f - subtex->height / static_cast<float>(height_pow2);

    C3D_TexInit(tex, width_pow2, height_pow2, GPU_RGB8);
    tex->border = 0xFFFFFFFF;
    C3D_TexSetWrap(tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
    memset(tex->data, 0, tex->size);

    return C2D_Image({tex, subtex});
}

void LoadThumbnail(ScreenshotInfo *info) {
    info->init_thumbnail();

    unsigned int error = loadbmp_to_texture(info->path_top, info->thumbnail.tex, ui::kThumbnailDownscale);
    info->has_thumbnail = !error;
}

void ThreadLoadThumbnails(void *arg) {
    auto initial_visible = screenshots_indexes;
    auto initial_hidden = screenshots_hidden;

    for (auto &indexes : std::vector<std::vector<size_t>>({initial_visible, initial_hidden})) {
        for (size_t i = 0; i < indexes.size() && run_thread; i++) {
            LoadThumbnail(&screenshots[indexes[i]]);

            loaded_thumbs = loaded_thumbs + 1;
        }
    }
}

void ThreadLoadScreenshot(void *arg) {
    while (run_thread) {
        svcWaitSynchronization(loadScreenshotRequest, U64_MAX);
        svcClearEvent(loadScreenshotRequest);

        size_t index = loading_screenshot_index;
        if (index != last_screenshot_index && index < screenshots.size()) {
            Screenshot *screenshot = screenshot_buffer[current_buffer];

            unsigned int error;
            if (screenshots[index].path_top_right.size() > 0) {
                error = loadbmp_to_texture(screenshots[index].path_top_right, screenshot->top_right.tex);
                if (error) {
                    screenshot->is_3d = false;
                } else {
                    screenshot->is_3d = true;
                }
            } else {
                screenshot->is_3d = false;
            }

            error = loadbmp_to_texture(screenshots[index].path_top, screenshot->top.tex);
            if (error) {
                memset(screenshot->top_right.tex->data, 0, screenshot->top_right.tex->size);
                memset(screenshot->top.tex->data, 0, screenshot->top.tex->size);
                screenshot->is_3d = false;
            }

            error = loadbmp_to_texture(screenshots[index].path_bottom, screenshot->bottom.tex);
            if (error) {
                memset(screenshot->bottom.tex->data, 0, screenshot->bottom.tex->size);
            }

            last_screenshot_index = index;
        }
    }
}

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

                if (screenshots.size() == 0 || screenshots.back().name != name) {
                    screenshots.push_back(ScreenshotInfo(name, tags::GetScreenshotTags(name)));
                }

                ScreenshotInfo &scrs = screenshots.back();
                switch (type) {
                    case TOP:
                        scrs.path_top = files[i];
                        break;
                    case TOP_RIGHT:
                        scrs.path_top_right = files[i];
                        break;
                    case BOTTOM:
                        scrs.path_bottom = files[i];
                        break;
                }

                break;
            }
        }
    }
}

void UpdateOrder() {
    std::list<size_t> filtered_screenshots;
    screenshots_hidden.clear();

    for (size_t i = 0; i < screenshots.size(); i++) {
        if ((tags::GetTagsFilter().size() == 0 || screenshots[i].has_all_tag(tags::GetTagsFilter())) && !screenshots[i].has_any_tag(tags::GetHiddenTags())) {
            filtered_screenshots.push_back(i);
        } else {
            screenshots_hidden.push_back(i);
        }
    }

    screenshots_indexes.clear();
    screenshots_indexes.reserve(filtered_screenshots.size());
    switch (screenshot_order) {
        case kNewer:
            std::reverse(filtered_screenshots.begin(), filtered_screenshots.end());
            // pass through
        case kOlder:
            std::copy(filtered_screenshots.begin(), filtered_screenshots.end(), std::back_inserter(screenshots_indexes));
            break;
        case kTags:
        case kTagsNewer:
            std::map<tags::tag_ptr, std::list<size_t>> index_groups = {{nullptr, {}}};
            std::set<tags::tag_ptr> processed_tags;
            std::vector<tags::tag_ptr> tags_order;

            if (screenshot_order == kTagsNewer) {
                std::reverse(filtered_screenshots.begin(), filtered_screenshots.end());
            }

            for (auto it = filtered_screenshots.begin(); it != filtered_screenshots.end(); ++it) {
                auto screenshot_tags = screenshots[*it].tags;
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
                        group.sort([](size_t s1, size_t s2) {
                            auto s1_tags = screenshots[s1].tags;
                            auto s2_tags = screenshots[s2].tags;
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

                        screenshots_indexes.insert(screenshots_indexes.end(), group.begin(), group.end());
                    }
                }

                screenshots_indexes.insert(screenshots_indexes.end(), index_groups[nullptr].begin(), index_groups[nullptr].end());
            } else {
                for (auto tag : tags_order) {
                    auto group = index_groups[tag];
                    screenshots_indexes.insert(screenshots_indexes.end(), group.begin(), group.end());
                }
            }
            break;
    }
}

void Init() {
    SearchScreenshots();
    UpdateOrder();

    for (int i = 0; i < num_buffers; i++) {
        screenshot_buffer[i] = new Screenshot({
            false,
            CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
            CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
            CreateImage(ui::kBottomScreenWidth, ui::kBottomScreenHeight),
        });
    }

    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    svcCreateEvent(&loadScreenshotRequest, RESET_ONESHOT);
    run_thread = true;

    thumbnailThread = threadCreate(ThreadLoadThumbnails, nullptr, kStackSize, prio - 1, -2, false);
    loadScreenshotThread = threadCreate(ThreadLoadScreenshot, nullptr, kStackSize, prio - 1, -2, false);
}

void Exit() {
    run_thread = false;
    threadJoin(thumbnailThread, U64_MAX);
    threadFree(thumbnailThread);
    svcSignalEvent(loadScreenshotRequest);
    threadJoin(loadScreenshotThread, U64_MAX);
    threadFree(loadScreenshotThread);
}

void Update() {
    if (load_screenshot_callback != nullptr) {
        if (next_screenshot_index == last_screenshot_index) {
            load_screenshot_callback(screenshot_buffer[current_buffer]);
            load_screenshot_callback = nullptr;
            last_buffer = current_buffer;
            current_buffer = (current_buffer + 1) % num_buffers;
        } else if (loading_screenshot_index == last_screenshot_index) {
            loading_screenshot_index = next_screenshot_index;
            svcSignalEvent(loadScreenshotRequest);
        }
    }
}

void Load(std::size_t index, void (*callback)(screenshot_ptr)) {
    if (index >= screenshots_indexes.size()) {
        callback(nullptr);
        return;
    }

    index = screenshots_indexes[index];
    if (index == next_screenshot_index) {
        if (load_screenshot_callback == nullptr) callback(screenshot_buffer[last_buffer]);
        return;
    }

    next_screenshot_index = index;
    load_screenshot_callback = callback;
}

size_t Count() { return screenshots_indexes.size(); }
size_t NumLoadedThumbnails() { return loaded_thumbs; }
bool FoundScreenshots() { return screenshots.size() > 0; }

info_ptr GetInfo(std::size_t index) {
    if (index >= screenshots_indexes.size()) return nullptr;
    return &screenshots[screenshots_indexes[index]];
}

const ScreenshotOrder GetOrder() { return screenshot_order; }
void SetOrder(ScreenshotOrder order) {
    screenshot_order = order;
    UpdateOrder();
}

void Delete(std::set<std::string> screenshot_names) {
    std::vector<ScreenshotInfo> new_screenshots;
    std::vector<ScreenshotInfo> deleted_screenshots;
    for (size_t i = 0; i < screenshots.size(); i++) {
        if (screenshot_names.contains(screenshots[i].name)) {
            deleted_screenshots.push_back(std::move(screenshots[i]));
        } else {
            new_screenshots.push_back(std::move(screenshots[i]));
        }
    }

    // Wait until the thumbnail thread has finished loading
    threadJoin(thumbnailThread, U64_MAX);

    screenshots = std::move(new_screenshots);

    for (auto &screenshot : deleted_screenshots) {
        try {
            if (screenshot.path_top != "") std::filesystem::remove(screenshot.path_top);
            if (screenshot.path_top_right != "") std::filesystem::remove(screenshot.path_top_right);
            if (screenshot.path_bottom != "") std::filesystem::remove(screenshot.path_bottom);
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

void ScreenshotInfo::init_thumbnail() { thumbnail = CreateImage(ui::kThumbnailWidth, ui::kThumbnailHeight); }

ScreenshotInfo::~ScreenshotInfo() {
    if (thumbnail.tex != nullptr || thumbnail.subtex != nullptr) {
        C3D_TexDelete(thumbnail.tex);
        delete thumbnail.tex;
        delete thumbnail.subtex;
    }
}

ScreenshotInfo::ScreenshotInfo(std::string name, const std::vector<tags::tag_ptr> &tags) : name(name), tags(tags), has_thumbnail(false) {}
ScreenshotInfo::ScreenshotInfo(ScreenshotInfo &&other)
    : name(std::move(other.name)),

      path_top(std::move(other.path_top)),
      path_top_right(std::move(other.path_top_right)),
      path_bottom(std::move(other.path_bottom)),

      tags(other.tags),
      has_thumbnail(other.has_thumbnail),

      thumbnail(std::move(other.thumbnail)) {
    other.thumbnail.tex = nullptr;
    other.thumbnail.subtex = nullptr;
}
}  // namespace screenshots
