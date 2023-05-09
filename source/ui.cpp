#include "ui.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <cstring>
#include <iostream>
#include <loadbmp.hpp>

#include "screenshots.hpp"

#define STACKSIZE (4 * 1024)

namespace ui {

C3D_RenderTarget *top_target;
C3D_RenderTarget *top_right_target;
C3D_RenderTarget *bottom_target;

u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
u32 clrGray = C2D_Color32(0x7F, 0x7F, 0x7F, 0xFF);
u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
u32 clrBackground = C2D_Color32(0x40, 0x40, 0x40, 0xFF);
u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0xFF);

screenshots::Screenshot selected_screenshot;
int selected_index = 0;
int last_loaded_thumbs = 0;
unsigned int ticks_since_last_input = kSelectionDebounceTicks;

bool show_ui = true;
bool is_3d = false;
bool changed_selection = true;
bool changed_screen = true;
bool pressed_exit_button = false;

int get_page_index(int x) { return (((x) / (kNRows * kNCols)) * (kNRows * kNCols)); }
int get_selected_page() { return get_page_index(selected_index); }
int get_selected_screenshot_position() { return (selected_index - get_selected_page()); }
void draw_bottom();
void draw_top();
void touchDownActions();

void init() {
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    top_right_target = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    if (kShowConsole) consoleInit(GFX_TOP, NULL);
}

void exit() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void input() {
    hidScanInput();

    if (!keysDown()) {
        ticks_since_last_input = ticks_since_last_input < kSelectionDebounceTicks ? ticks_since_last_input + 1 : kSelectionDebounceTicks;
    } else {
        ticks_since_last_input = 0;
    }

    if (keysDown() & KEY_TOUCH) {
        touchDownActions();
        ticks_since_last_input = kSelectionDebounceTicks;  // Debounce is not used for touch actions
    }

    if (keysHeld() & KEY_START) {
        pressed_exit_button = true;
        return;
    }

    if (keysDown() & KEY_DLEFT) {
        selected_index = std::max(0, selected_index - 1);
        changed_selection = true;
    }

    if (keysDown() & KEY_DRIGHT) {
        selected_index = std::min(screenshots::size() - 1, (size_t)selected_index + 1);
        changed_selection = true;
    }

    if (keysDown() & KEY_DUP && get_selected_screenshot_position() - kNCols >= 0) {
        selected_index -= kNCols;
        changed_selection = true;
    }

    if (keysDown() & KEY_DDOWN && get_selected_screenshot_position() < kNRows * (kNCols - 1)) {
        selected_index += kNCols;
        changed_selection = true;
    }

    if (keysDown() & KEY_L && get_selected_page() > 0) {
        selected_index = std::max(0, selected_index - kNCols * kNRows);
        changed_selection = true;
    }

    if (keysDown() & KEY_R && get_selected_page() < get_page_index(screenshots::size() - 1)) {
        selected_index = std::min(screenshots::size() - 1, (size_t)selected_index + kNCols * kNCols);
        changed_selection = true;
    }

    if (keysDown() & KEY_SELECT) {
        show_ui = !show_ui;
        changed_screen = true;
    }

    if (last_loaded_thumbs != screenshots::num_loaded_thumbs()) {
        last_loaded_thumbs = screenshots::num_loaded_thumbs();
        if (show_ui) changed_screen = true;
    }

    if (changed_selection && ticks_since_last_input >= kSelectionDebounceTicks) {
        selected_screenshot = screenshots::load(selected_index);
        changed_selection = false;
        changed_screen = true;
        ticks_since_last_input = 0;
    }
}

void render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    if (changed_selection || changed_screen) {
        draw_top();
        draw_bottom();
    }

    changed_screen = false;
    C3D_FrameEnd(0);
}

bool pressed_exit() { return pressed_exit_button; }

void draw_right_arrow(unsigned int x, unsigned int y, unsigned int scale) {
    C2D_DrawTriangle(x - scale / 4 + 3, y + scale / 2, clrBlack, x - scale / 4 + 3, y - scale / 2, clrBlack, x + scale / 2, y, clrBlack, 0);
}

void draw_left_arrow(unsigned int x, unsigned int y, unsigned int scale) {
    C2D_DrawTriangle(x + scale / 4 - 3, y + scale / 2, clrBlack, x + scale / 4 - 3, y - scale / 2, clrBlack, x - scale / 2, y, clrBlack, 0);
}

void draw_down_arrow(unsigned int x, unsigned int y, unsigned int scale) {
    C2D_DrawTriangle(x + scale / 2, y - scale / 4, clrBlack, x - scale / 2 - 0.5, y - scale / 4, clrBlack, x, y + scale / 4, clrBlack, 0);
}

void draw_interface() {
    C2D_DrawRectSolid(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * ((get_selected_screenshot_position()) % kNCols) - kSelectionOutline,
                      kVMargin + (kThumbnailHeight + kThumbnailSpacing) * ((int)((get_selected_screenshot_position()) / kNRows)) - kSelectionOutline, 0,
                      kThumbnailWidth + kSelectionOutline * 2, kThumbnailHeight + kSelectionOutline * 2, clrWhite);

    unsigned int i = get_selected_page();
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::size()) break;

            if (screenshots::get_info(i).hasThumbnail) {
                C2D_DrawImageAt(screenshots::get_info(i).thumbnail, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c,
                                kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, 0);
            } else {
                C2D_DrawRectSolid(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, 0,
                                  kThumbnailWidth, kThumbnailHeight, clrGray);
            }

            i++;
        }
    }

    C2D_DrawRectSolid(0, kBottomScreenHeight - kNavbarHeight, 0, kNavbarArrowWidth, kNavbarHeight, clrGray);
    C2D_DrawRectSolid(kBottomScreenWidth - kNavbarArrowWidth, kBottomScreenHeight - kNavbarHeight, 0, kNavbarArrowWidth, kNavbarHeight, clrGray);
    C2D_DrawRectSolid(kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, 0, kNavbarHideButtonWidth, kNavbarHeight, clrGray);

    draw_left_arrow(kNavbarIconMargin + kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);
    draw_right_arrow(kBottomScreenWidth - kNavbarIconMargin - kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);

    draw_down_arrow(kBottomScreenWidth / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);
}

void draw_bottom() {
    C2D_TargetClear(bottom_target, clrBackground);
    C2D_SceneBegin(bottom_target);

    if (show_ui)
        draw_interface();
    else
        C2D_DrawImageAt(selected_screenshot.bottom, 0, 0, 0);
}

void draw_top() {
    if (kShowConsole) return;

    gfxSet3D(selected_screenshot.is_3d);

    C2D_TargetClear(top_target, clrClear);
    C2D_SceneBegin(top_target);
    C2D_DrawImageAt(selected_screenshot.top, 0, 0, 0);

    if (selected_screenshot.is_3d) {
        C2D_TargetClear(top_right_target, clrClear);
        C2D_SceneBegin(top_right_target);
        C2D_DrawImageAt(selected_screenshot.top_right, 0, 0, 0);
    }
}

bool touchedInRect(touchPosition touch, int x, int y, int w, int h) { return touch.px > x && touch.px < x + w && touch.py > y && touch.py < y + h; }

void touchDownActions() {
    if (show_ui == false) {
        show_ui = true;
        changed_screen = true;
        return;
    }

    // Read the touch screen coordinates
    touchPosition touch;
    hidTouchRead(&touch);

    // Next page
    if (touchedInRect(touch, kBottomScreenWidth - kNavbarArrowWidth, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight)) {
        selected_index = std::min(screenshots::size() - 1, (size_t)selected_index + kNCols * kNCols);
        changed_selection = true;
    }

    // Previous page
    if (touchedInRect(touch, 0, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight)) {
        selected_index = std::max(0, selected_index - kNCols * kNRows);
        changed_selection = true;
    }

    // Hide UI
    if (touchedInRect(touch, kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kNavbarHideButtonWidth, kNavbarHeight)) {
        show_ui = false;
        changed_screen = true;
    }

    // Select screenshot
    unsigned int i = get_selected_page();
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::size()) break;

            if (touchedInRect(touch, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r,
                              kThumbnailWidth, kThumbnailHeight)) {
                selected_index = i;
                changed_selection = true;
                return;
            }

            i++;
        }
    }
}

}  // namespace ui
