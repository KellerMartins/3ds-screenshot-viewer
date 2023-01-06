///////////////////////////////////////
//           SDMC example            //
///////////////////////////////////////

// this example shows you how to load a binary image file from the SD card and display it on the lower screen
// for this to work you should copy test.bin to same folder as your .3dsx
// this file was generated with GIMP by saving a 240x320 image to raw RGB
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define LOADBMP_IMPLEMENTATION
#include "loadbmp.h"
#include <3ds.h>

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HEIGHT 240
#define TOP_SCREEN_SIZE (TOP_SCREEN_WIDTH * TOP_SCREEN_HEIGHT * 3)

#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_SIZE (BOTTOM_SCREEN_WIDTH * BOTTOM_SCREEN_HEIGHT * 3)

#define THUMBNAIL_DOWNSCALE 4

#define THUMBNAIL_SPACING 4
#define THUMBNAIL_WIDTH (TOP_SCREEN_WIDTH / THUMBNAIL_DOWNSCALE)
#define THUMBNAIL_HEIGHT (TOP_SCREEN_HEIGHT / THUMBNAIL_DOWNSCALE)
#define THUMBNAIL_SIZE (THUMBNAIL_WIDTH * THUMBNAIL_HEIGHT * 3)

#define N_ROWS ((BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT) / THUMBNAIL_HEIGHT)
#define N_COLS (BOTTOM_SCREEN_WIDTH / (THUMBNAIL_WIDTH + THUMBNAIL_SPACING))

#define INDEX_PAGE(X) (((X) / (N_ROWS * N_COLS)) * (N_ROWS * N_COLS))

#define SELECTION_OUTLINE 2
#define SELECTED_PAGE INDEX_PAGE(selected)
#define SELECTED_POSITION (selected - SELECTED_PAGE)

#define NAVBAR_HEIGHT 24
#define NAVBAR_ARROW_WIDTH 64
#define NAVBAR_BUTTONS_SPACING 1
#define NAVBAR_ICON_MARGIN 24
#define NAVBAR_ICON_SPACING 4
#define NAVBAR_ICON_SCALE (NAVBAR_HEIGHT - NAVBAR_ICON_SPACING * 2)
#define NAVBAR_HIDE_BUTTON_WIDTH (BOTTOM_SCREEN_WIDTH - (NAVBAR_ARROW_WIDTH + NAVBAR_BUTTONS_SPACING) * 2)

#define STACKSIZE (4 * 1024)

#define SHOW_CONSOLE false

// this will contain the data read from SDMC
u8 *top_buffer;
u8 *top_right_buffer;
u8 *bottom_buffer;

int selected = 0;
int num_loaded_thumbs = 0;
bool show_ui = true;
bool is3D = false;

struct screenshot {
    std::string name;
    std::string path_top;
    std::string path_top_right;
    std::string path_bot;
    u8 *thumbnail;
};

enum SCRS_TYPE {
    TOP = 0,
    TOP_RIGHT = 1,
    BOTTOM = 2,
};

std::string suffixes[] = {
    "_top.bmp",
    "_top_right.bmp",
    "_bot.bmp"};

std::vector<screenshot> screenshots;

volatile bool runThread = true;
void threadThumbnails(void *arg) {

    for (size_t i = 0; i < screenshots.size() && runThread; i++) {
        loadbmp_to_framebuffer(screenshots[i].path_top, screenshots[i].thumbnail, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, THUMBNAIL_DOWNSCALE);
        num_loaded_thumbs++;
    }
}

void find_screenshots() {
    auto files = std::vector<std::string>();

    for (const auto &entry : std::filesystem::directory_iterator("/luma/screenshots"))
        files.push_back(entry.path());

    std::sort(files.begin(), files.end());

    for (size_t i = 0; i < files.size(); i++) {
        SCRS_TYPE type;
        std::string name;

        for (size_t s = 0; s < sizeof(suffixes) / sizeof(*suffixes); s++) {
            if (files[i].ends_with(suffixes[s])) {
                type = (SCRS_TYPE)s;
                name = files[i].substr(0, files[i].size() - suffixes[s].size());

                if (screenshots.size() == 0 || screenshots.back().name != name) {
                    screenshot newScreenshot;
                    newScreenshot.name = name;
                    newScreenshot.thumbnail = (unsigned char *)malloc(THUMBNAIL_SIZE);
                    memset(newScreenshot.thumbnail, 127, THUMBNAIL_SIZE);

                    std::cout << name << "\n";

                    screenshots.push_back(newScreenshot);
                }

                screenshot &scrs = screenshots.back();
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

void copy_thumbnail_to_fb(u8 *thumbnail_buffer, u8 *fb, unsigned int offset_x, unsigned int offset_y) {
    for (int x = 0; x < THUMBNAIL_WIDTH; x++) {
        memcpy(fb + ((x + offset_x) * BOTTOM_SCREEN_HEIGHT + offset_y) * 3, thumbnail_buffer + x * THUMBNAIL_HEIGHT * 3, THUMBNAIL_HEIGHT * 3);
    }
}

void draw_grayscale_rect_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int width, unsigned int height) {
    for (int x = 0; x < (int)width; x++) {
        memset(fb + ((x + offset_x) * BOTTOM_SCREEN_HEIGHT + offset_y) * 3, gray, height * 3);
    }
}

void draw_right_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = 0; (scale - x * 2) > 0; x++) {
        memset(fb + ((x + offset_x) * BOTTOM_SCREEN_HEIGHT + (offset_y + x)) * 3, gray, (scale - x * 2) * 3);
    }
}

void draw_left_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = scale / 2; x >= 0; x--) {
        memset(fb + ((x + offset_x) * BOTTOM_SCREEN_HEIGHT + (offset_y + (scale / 2 - x))) * 3, gray, (x * 2) * 3);
    }
}

void draw_down_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = 0; x <= (int)scale / 2; x++) {
        memset(fb + ((x + offset_x) * BOTTOM_SCREEN_HEIGHT + (offset_y + scale / 2 - x)) * 3, gray, x * 3);
    }
    for (int x = 0; x < (int)scale / 2; x++) {
        memset(fb + ((x + offset_x + scale / 2 + 1) * BOTTOM_SCREEN_HEIGHT + (offset_y + x)) * 3, gray, (scale / 2 - x) * 3);
    }
}

void draw_interface() {
    memset(bottom_buffer, 64, BOTTOM_SCREEN_SIZE);

    const int h_margin = (BOTTOM_SCREEN_WIDTH - N_COLS * THUMBNAIL_WIDTH - (N_COLS - 1) * THUMBNAIL_SPACING) / 2;
    const int v_margin = ((BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT) - N_ROWS * THUMBNAIL_HEIGHT - (N_ROWS - 1) * THUMBNAIL_SPACING) / 2;

    draw_grayscale_rect_to_fb(bottom_buffer, 255,
                              h_margin + (THUMBNAIL_WIDTH + THUMBNAIL_SPACING) * ((SELECTED_POSITION) % N_COLS) - SELECTION_OUTLINE,
                              v_margin + (THUMBNAIL_HEIGHT + THUMBNAIL_SPACING) * (N_ROWS - 1 - ((SELECTED_POSITION) / N_ROWS)) + NAVBAR_HEIGHT - SELECTION_OUTLINE,
                              THUMBNAIL_WIDTH + SELECTION_OUTLINE * 2, THUMBNAIL_HEIGHT + SELECTION_OUTLINE * 2);

    unsigned int i = SELECTED_PAGE;
    for (int r = N_ROWS - 1; r >= 0; r--) {
        for (int c = 0; c < N_COLS; c++) {

            if (i >= screenshots.size())
                break;

            if (screenshots[i].thumbnail != nullptr)

                copy_thumbnail_to_fb(screenshots[i].thumbnail, bottom_buffer,
                                     h_margin + (THUMBNAIL_WIDTH + THUMBNAIL_SPACING) * c,
                                     v_margin + (THUMBNAIL_HEIGHT + THUMBNAIL_SPACING) * (r) + NAVBAR_HEIGHT);

            i++;
        }
    }

    draw_grayscale_rect_to_fb(bottom_buffer, 127, 0, 0, NAVBAR_ARROW_WIDTH, NAVBAR_HEIGHT);
    draw_grayscale_rect_to_fb(bottom_buffer, 127, BOTTOM_SCREEN_WIDTH - NAVBAR_ARROW_WIDTH, 0, NAVBAR_ARROW_WIDTH, NAVBAR_HEIGHT);
    draw_grayscale_rect_to_fb(bottom_buffer, 127, NAVBAR_ARROW_WIDTH + NAVBAR_BUTTONS_SPACING, 0, NAVBAR_HIDE_BUTTON_WIDTH, NAVBAR_HEIGHT);

    draw_left_arrow_to_fb(bottom_buffer, 0, NAVBAR_ICON_MARGIN, NAVBAR_ICON_SPACING, NAVBAR_ICON_SCALE);
    draw_right_arrow_to_fb(bottom_buffer, 0, BOTTOM_SCREEN_WIDTH - NAVBAR_ICON_MARGIN - NAVBAR_ICON_SCALE / 2, NAVBAR_ICON_SPACING, NAVBAR_ICON_SCALE);

    draw_down_arrow_to_fb(bottom_buffer, 0, NAVBAR_ARROW_WIDTH + NAVBAR_BUTTONS_SPACING + (NAVBAR_HIDE_BUTTON_WIDTH - NAVBAR_ICON_SCALE) / 2, NAVBAR_ICON_SPACING + NAVBAR_ICON_SCALE / 4, NAVBAR_ICON_SCALE);
}

void draw_top_screenshot() {
    unsigned int error;

    if (screenshots[selected].path_top_right.size() > 0) {
        error = loadbmp_to_framebuffer(screenshots[selected].path_top_right, top_right_buffer, TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT, 1);
        if (error) {
            is3D = false;
        } else {
            is3D = true;
        }
    } else {
        is3D = false;
    }

    error = loadbmp_to_framebuffer(screenshots[selected].path_top, top_buffer, TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT, 1);
    if (error) {
        memset(top_buffer, 0, TOP_SCREEN_SIZE);
        is3D = false;
    }
}

void draw_bottom_screenshot() {
    memset(bottom_buffer, 0, BOTTOM_SCREEN_SIZE);

    unsigned int error = loadbmp_to_framebuffer(screenshots[selected].path_bot, bottom_buffer, BOTTOM_SCREEN_WIDTH, BOTTOM_SCREEN_HEIGHT, 1);
    if (error)
        memset(bottom_buffer, 0, BOTTOM_SCREEN_SIZE);
}

void draw_bottom() {
    if (show_ui)
        draw_interface();
    else
        draw_bottom_screenshot();

    memcpy(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), bottom_buffer, BOTTOM_SCREEN_SIZE);
}

void draw_top() {
    draw_top_screenshot();

    gfxSet3D(is3D);
    if (gfxIs3D()) {
        memcpy(gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), top_right_buffer, TOP_SCREEN_SIZE);
    }

    memcpy(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), top_buffer, TOP_SCREEN_SIZE);
    memcpy(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), bottom_buffer, BOTTOM_SCREEN_SIZE);
}

int main(int argc, char **argv) {
    top_buffer = (unsigned char *)malloc(TOP_SCREEN_SIZE);
    top_right_buffer = (unsigned char *)malloc(TOP_SCREEN_SIZE);
    bottom_buffer = (unsigned char *)malloc(BOTTOM_SCREEN_SIZE);

    gfxInitDefault(); // makes displaying to screen easier
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    if (SHOW_CONSOLE)
        consoleInit(GFX_TOP, NULL);

    find_screenshots();

    Thread thumbnailThread;
    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    thumbnailThread = threadCreate(threadThumbnails, nullptr, STACKSIZE, prio - 1, -2, false);

    bool firstRender = true;
    int lastLoadedThumbs = 0;

    while (aptMainLoop()) {
        bool changedTop = firstRender, changedBottom = firstRender;
        firstRender = false;

        // exit when user hits start
        hidScanInput();
        if (keysHeld() & KEY_START)
            break;

        if (keysDown() & KEY_DLEFT) {
            selected = std::max(0, selected - 1);
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_DRIGHT) {
            selected = std::min(screenshots.size() - 1, (size_t)selected + 1);
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_DUP && SELECTED_POSITION - N_COLS >= 0) {
            selected -= N_COLS;
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_DDOWN && SELECTED_POSITION < N_ROWS * (N_COLS - 1)) {
            selected += N_COLS;
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_L && SELECTED_PAGE > 0) {
            selected -= N_COLS * N_ROWS;
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_R && SELECTED_PAGE < (int)INDEX_PAGE(screenshots.size() - 1)) {
            selected = std::min(screenshots.size() - 1, (size_t)selected + N_COLS * N_COLS);
            changedTop = changedBottom = true;
        }

        if (keysDown() & KEY_SELECT) {
            show_ui = !show_ui;
            changedBottom = true;
        }

        if (lastLoadedThumbs < num_loaded_thumbs) {
            lastLoadedThumbs = num_loaded_thumbs;
            if (show_ui)
                changedBottom = true;
        }

        if (SHOW_CONSOLE)
            changedTop = false;

        if (changedTop)
            draw_top();
        if (changedBottom)
            draw_bottom();

        gfxFlushBuffers();
        gspWaitForVBlank();

        if (changedTop)
            gfxScreenSwapBuffers(GFX_TOP, is3D);
        if (changedBottom)
            gfxScreenSwapBuffers(GFX_BOTTOM, is3D);
    }

    runThread = false;
    threadJoin(thumbnailThread, U64_MAX);
    threadFree(thumbnailThread);

    // closing all services even more so
    gfxExit();
    return 0;
}
