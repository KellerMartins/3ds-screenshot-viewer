#include "ui.hpp"

#include <3ds.h>

#include <cstring>
#include <loadbmp.hpp>

#include "screenshots.hpp"

#define STACKSIZE (4 * 1024)

namespace ui {

u8 *top_buffer;
u8 *top_right_buffer;
u8 *bottom_buffer;

int selected_screenshot = 0;
int last_loaded_thumbs = 0;

bool show_ui = true;
bool is_3d = false;
bool changed_top = true;
bool changed_bottom = true;
bool pressed_exit_button = false;

int get_page_index(int x) { return (((x) / (kNRows * kNCols)) * (kNRows * kNCols)); }
int get_selected_page() { return get_page_index(selected_screenshot); }
int get_selected_screenshot_position() { return (selected_screenshot - get_selected_page()); }
void draw_bottom();
void draw_top();

void init() {
    top_buffer = (unsigned char *)malloc(kTopScreenSize);
    top_right_buffer = (unsigned char *)malloc(kTopScreenSize);
    bottom_buffer = (unsigned char *)malloc(kBottomScreenSize);

    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    if (kShowConsole) consoleInit(GFX_TOP, NULL);
}

void exit() { gfxExit(); }

void input() {
    // exit when user hits start
    hidScanInput();
    if (keysHeld() & KEY_START) {
        pressed_exit_button = true;
        return;
    }

    if (keysDown() & KEY_DLEFT) {
        selected_screenshot = std::max(0, selected_screenshot - 1);
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_DRIGHT) {
        selected_screenshot = std::min(screenshots::size() - 1, (size_t)selected_screenshot + 1);
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_DUP && get_selected_screenshot_position() - kNCols >= 0) {
        selected_screenshot -= kNCols;
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_DDOWN && get_selected_screenshot_position() < kNRows * (kNCols - 1)) {
        selected_screenshot += kNCols;
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_L && get_selected_page() > 0) {
        selected_screenshot -= kNCols * kNRows;
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_R && get_selected_page() < get_page_index(screenshots::size() - 1)) {
        selected_screenshot = std::min(screenshots::size() - 1, (size_t)selected_screenshot + kNCols * kNCols);
        changed_top = changed_bottom = true;
    }

    if (keysDown() & KEY_SELECT) {
        show_ui = !show_ui;
        changed_bottom = true;
    }

    if (last_loaded_thumbs < screenshots::num_loaded_thumbs()) {
        last_loaded_thumbs = screenshots::num_loaded_thumbs();
        if (show_ui) changed_bottom = true;
    }

    if (kShowConsole) changed_top = false;
}

void render() {
    if (changed_top) draw_top();
    if (changed_bottom) draw_bottom();

    gfxFlushBuffers();
    gspWaitForVBlank();

    if (changed_top) gfxScreenSwapBuffers(GFX_TOP, is_3d);
    if (changed_bottom) gfxScreenSwapBuffers(GFX_BOTTOM, is_3d);

    changed_top = changed_bottom = false;
}

bool pressed_exit() { return pressed_exit_button; }

void copy_thumbnail_to_fb(u8 *thumbnail_buffer, u8 *fb, unsigned int offset_x, unsigned int offset_y) {
    for (int x = 0; x < kThumbnailWidth; x++) {
        memcpy(fb + ((x + offset_x) * kBottomScreenHeight + offset_y) * 3, thumbnail_buffer + x * kThumbnailHeight * 3, kThumbnailHeight * 3);
    }
}

void draw_grayscale_rect_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int width, unsigned int height) {
    for (int x = 0; x < (int)width; x++) {
        memset(fb + ((x + offset_x) * kBottomScreenHeight + offset_y) * 3, gray, height * 3);
    }
}

void draw_right_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = 0; (scale - x * 2) > 0; x++) {
        memset(fb + ((x + offset_x) * kBottomScreenHeight + (offset_y + x)) * 3, gray, (scale - x * 2) * 3);
    }
}

void draw_left_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = scale / 2; x >= 0; x--) {
        memset(fb + ((x + offset_x) * kBottomScreenHeight + (offset_y + (scale / 2 - x))) * 3, gray, (x * 2) * 3);
    }
}

void draw_down_arrow_to_fb(u8 *fb, u8 gray, unsigned int offset_x, unsigned int offset_y, unsigned int scale) {
    for (int x = 0; x <= (int)scale / 2; x++) {
        memset(fb + ((x + offset_x) * kBottomScreenHeight + (offset_y + scale / 2 - x)) * 3, gray, x * 3);
    }
    for (int x = 0; x < (int)scale / 2; x++) {
        memset(fb + ((x + offset_x + scale / 2 + 1) * kBottomScreenHeight + (offset_y + x)) * 3, gray, (scale / 2 - x) * 3);
    }
}

void draw_interface() {
    memset(bottom_buffer, 64, kBottomScreenSize);

    const int h_margin = (kBottomScreenWidth - kNCols * kThumbnailWidth - (kNCols - 1) * kThumbnailSpacing) / 2;
    const int v_margin = ((kBottomScreenHeight - kNavbarHeight) - kNRows * kThumbnailHeight - (kNRows - 1) * kThumbnailSpacing) / 2;

    draw_grayscale_rect_to_fb(
        bottom_buffer, 255, h_margin + (kThumbnailWidth + kThumbnailSpacing) * ((get_selected_screenshot_position()) % kNCols) - kSelectionOutline,
        v_margin + (kThumbnailHeight + kThumbnailSpacing) * (kNRows - 1 - ((get_selected_screenshot_position()) / kNRows)) + kNavbarHeight - kSelectionOutline,
        kThumbnailWidth + kSelectionOutline * 2, kThumbnailHeight + kSelectionOutline * 2);

    unsigned int i = get_selected_page();
    for (int r = kNRows - 1; r >= 0; r--) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::size()) break;

            if (screenshots::get(i).thumbnail != nullptr)

                copy_thumbnail_to_fb(screenshots::get(i).thumbnail, bottom_buffer, h_margin + (kThumbnailWidth + kThumbnailSpacing) * c,
                                     v_margin + (kThumbnailHeight + kThumbnailSpacing) * (r) + kNavbarHeight);

            i++;
        }
    }

    draw_grayscale_rect_to_fb(bottom_buffer, 127, 0, 0, kNavbarArrowWidth, kNavbarHeight);
    draw_grayscale_rect_to_fb(bottom_buffer, 127, kBottomScreenWidth - kNavbarArrowWidth, 0, kNavbarArrowWidth, kNavbarHeight);
    draw_grayscale_rect_to_fb(bottom_buffer, 127, kNavbarArrowWidth + kNavbarButtonsSpacing, 0, kNavbarHideButtonWidth, kNavbarHeight);

    draw_left_arrow_to_fb(bottom_buffer, 0, kNavbarIconMargin, kNavbarIconSpacing, kNavbarIconScale);
    draw_right_arrow_to_fb(bottom_buffer, 0, kBottomScreenWidth - kNavbarIconMargin - kNavbarIconScale / 2, kNavbarIconSpacing, kNavbarIconScale);

    draw_down_arrow_to_fb(bottom_buffer, 0, kNavbarArrowWidth + kNavbarButtonsSpacing + (kNavbarHideButtonWidth - kNavbarIconScale) / 2,
                          kNavbarIconSpacing + kNavbarIconScale / 4, kNavbarIconScale);
}

void draw_top_screenshot() {
    unsigned int error;

    if (screenshots::get(selected_screenshot).path_top_right.size() > 0) {
        error = loadbmp_to_framebuffer(screenshots::get(selected_screenshot).path_top_right, top_right_buffer, kTopScreenWidth, kTopScreenHeight, 1);
        if (error) {
            is_3d = false;
        } else {
            is_3d = true;
        }
    } else {
        is_3d = false;
    }

    error = loadbmp_to_framebuffer(screenshots::get(selected_screenshot).path_top, top_buffer, kTopScreenWidth, kTopScreenHeight, 1);
    if (error) {
        memset(top_buffer, 0, kTopScreenSize);
        is_3d = false;
    }
}

void draw_bottom_screenshot() {
    memset(bottom_buffer, 0, kBottomScreenSize);

    unsigned int error = loadbmp_to_framebuffer(screenshots::get(selected_screenshot).path_bot, bottom_buffer, kBottomScreenWidth, kBottomScreenHeight, 1);
    if (error) memset(bottom_buffer, 0, kBottomScreenSize);
}

void draw_bottom() {
    if (show_ui)
        draw_interface();
    else
        draw_bottom_screenshot();

    memcpy(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), bottom_buffer, kBottomScreenSize);
}

void draw_top() {
    draw_top_screenshot();

    gfxSet3D(is_3d);
    if (gfxIs3D()) {
        memcpy(gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), top_right_buffer, kTopScreenSize);
    }

    memcpy(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), top_buffer, kTopScreenSize);
    memcpy(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), bottom_buffer, kBottomScreenSize);
}

}  // namespace ui
