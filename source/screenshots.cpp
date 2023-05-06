#include "screenshots.hpp"

#include <3ds.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "loadbmp.hpp"
#include "ui.hpp"

namespace screenshots {

enum SCRS_TYPE {
    TOP = 0,
    TOP_RIGHT = 1,
    BOTTOM = 2,
};

std::string suffixes[] = {"_top.bmp", "_top_right.bmp", "_bot.bmp"};

std::vector<Screenshot> screenshots;
int loaded_thumbs = 0;

volatile bool run_thread = true;
Thread thumbnailThread;

void threadThumbnails(void *arg) {
    for (size_t i = 0; i < screenshots.size() && run_thread; i++) {
        loadbmp_to_framebuffer(screenshots[i].path_top, screenshots[i].thumbnail, ui::kThumbnailWidth, ui::kThumbnailHeight, ui::kThumbnailDownscale);
        loaded_thumbs++;
    }
}

void find() {
    auto files = std::vector<std::string>();

    for (const auto &entry : std::filesystem::directory_iterator("/luma/screenshots")) files.push_back(entry.path());

    std::sort(files.begin(), files.end());

    for (size_t i = 0; i < files.size(); i++) {
        SCRS_TYPE type;
        std::string name;

        for (size_t s = 0; s < sizeof(suffixes) / sizeof(*suffixes); s++) {
            if (files[i].ends_with(suffixes[s])) {
                type = (SCRS_TYPE)s;
                name = files[i].substr(0, files[i].size() - suffixes[s].size());

                if (screenshots.size() == 0 || screenshots.back().name != name) {
                    Screenshot newScreenshot;
                    newScreenshot.name = name;
                    newScreenshot.thumbnail = new unsigned char[ui::kThumbnailSize];
                    memset(newScreenshot.thumbnail, 127, ui::kThumbnailSize);

                    std::cout << name << "\n";

                    screenshots.push_back(newScreenshot);
                }

                Screenshot &scrs = screenshots.back();
                switch (type) {
                    case TOP:
                        scrs.path_top = files[i];
                        break;
                    case TOP_RIGHT:
                        scrs.path_top_right = files[i];
                        break;
                    case BOTTOM:
                        scrs.path_bot = files[i];
                        break;
                }

                break;
            }
        }
    }
}

#define STACKSIZE (4 * 1024)
void load_start() {
    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    run_thread = true;
    thumbnailThread = threadCreate(threadThumbnails, nullptr, STACKSIZE, prio - 1, -2, false);
}

void load_stop() {
    run_thread = false;
    threadJoin(thumbnailThread, U64_MAX);
    threadFree(thumbnailThread);
}

const Screenshot get(std::size_t index) { return screenshots[index]; }
size_t size() { return screenshots.size(); }
int num_loaded_thumbs() { return loaded_thumbs; }

}  // namespace screenshots
