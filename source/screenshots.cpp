#include "screenshots.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
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

std::string suffixes[] = {"_top.bmp", "_top_right.bmp", "_bot.bmp"};

std::vector<ScreenshotInfo> screenshots;
int loaded_thumbs = 0;

volatile bool run_thread = true;
Thread thumbnailThread;

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

void ThreadThumbnails(void *arg) {
    for (size_t i = 0; i < screenshots.size() && run_thread; i++) {
        screenshots[i].thumbnail = CreateImage(ui::kThumbnailWidth, ui::kThumbnailHeight);

        unsigned int error =
            loadbmp_to_texture(screenshots[i].path_top, screenshots[i].thumbnail.tex, ui::kThumbnailWidth, ui::kThumbnailHeight, ui::kThumbnailDownscale);
        screenshots[i].has_thumbnail = !error;

        loaded_thumbs++;
    }
}

void Init() {
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

#define STACKSIZE (4 * 1024)
void LoadThumbnailsStart() {
    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    run_thread = true;
    thumbnailThread = threadCreate(ThreadThumbnails, nullptr, STACKSIZE, prio - 1, -2, false);
}

void LoadThumbnailsStop() {
    run_thread = false;
    threadJoin(thumbnailThread, U64_MAX);
    threadFree(thumbnailThread);
}

const Screenshot Load(std::size_t index) {
    static Screenshot screenshot = Screenshot({
        false,
        CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
        CreateImage(ui::kTopScreenWidth, ui::kTopScreenHeight),
        CreateImage(ui::kBottomScreenWidth, ui::kBottomScreenHeight),
    });

    unsigned int error;
    if (screenshots[index].path_top_right.size() > 0) {
        error = loadbmp_to_texture(screenshots[index].path_top_right, screenshot.top_right.tex, ui::kTopScreenWidth, ui::kTopScreenHeight);
        if (error) {
            screenshot.is_3d = false;
        } else {
            screenshot.is_3d = true;
        }
    } else {
        screenshot.is_3d = false;
    }

    error = loadbmp_to_texture(screenshots[index].path_top, screenshot.top.tex, ui::kTopScreenWidth, ui::kTopScreenHeight);
    if (error) {
        memset(screenshot.top_right.tex->data, 0, screenshot.top_right.tex->size);
        memset(screenshot.top.tex->data, 0, screenshot.top.tex->size);
        screenshot.is_3d = false;
    }

    error = loadbmp_to_texture(screenshots[index].path_bottom, screenshot.bottom.tex, ui::kBottomScreenWidth, ui::kBottomScreenHeight);
    if (error) {
        memset(screenshot.bottom.tex->data, 0, screenshot.bottom.tex->size);
    }

    return screenshot;
}
const ScreenshotInfo GetInfo(std::size_t index) { return screenshots[index]; }
size_t Count() { return screenshots.size(); }
size_t NumLoadedThumbnails() { return loaded_thumbs; }

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
}  // namespace screenshots
