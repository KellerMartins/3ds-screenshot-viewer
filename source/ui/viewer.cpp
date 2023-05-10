#include "ui/viewer.hpp"

#include <3ds.h>
#include <citro2d.h>

#include "screenshots.hpp"
#include "ui/tags_menu.hpp"

namespace ui::viewer {

const int kThumbnailSpacing = 4;
const int kNavbarHeight = 24;
const int kNavbarArrowWidth = 64;
const int kNavbarButtonsSpacing = 1;
const int kNavbarIconMargin = 24;
const int kNavbarIconSpacing = 4;
const int kNavbarIconScale = (kNavbarHeight - kNavbarIconSpacing * 2);
const int kNavbarHideButtonWidth = (kBottomScreenWidth - (kNavbarArrowWidth + kNavbarButtonsSpacing) * 2);

const int kNRows = ((kBottomScreenHeight - kNavbarHeight) / kThumbnailHeight);
const int kNCols = (kBottomScreenWidth / (kThumbnailWidth + kThumbnailSpacing));

const int kHMargin = (kBottomScreenWidth - kNCols * kThumbnailWidth - (kNCols - 1) * kThumbnailSpacing) / 2;
const int kVMargin = ((kBottomScreenHeight - kNavbarHeight) - kNRows * kThumbnailHeight - (kNRows - 1) * kThumbnailSpacing) / 2;

const int kSelectionOutline = 2;

const unsigned int kSelectionDebounceTicks = 20;

const u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
const u32 clrGray = C2D_Color32(0x7F, 0x7F, 0x7F, 0xFF);
const u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
const u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0xFF);

screenshots::Screenshot selected_screenshot;
int selected_index = 0;
int last_loaded_thumbs = 0;
unsigned int ticks_since_last_input = kSelectionDebounceTicks;

bool show_ui = true;
bool is_3d = false;
bool changed_selection = true;
bool changed_screen = true;

int get_page_index(int x) { return (((x) / (kNRows * kNCols)) * (kNRows * kNCols)); }
int get_selected_page() { return get_page_index(selected_index); }
int get_selected_screenshot_position() { return (selected_index - get_selected_page()); }
void draw_bottom();
void draw_top();
void touchDownActions();

void open() {
    changed_screen = true;

    set_ui_functions(input, render);
};

void input() {
    if (!keysDown()) {
        ticks_since_last_input = ticks_since_last_input < kSelectionDebounceTicks ? ticks_since_last_input + 1 : kSelectionDebounceTicks;
    } else {
        ticks_since_last_input = 0;
    }

    if (keysDown() & KEY_TOUCH) {
        touchDownActions();
        ticks_since_last_input = kSelectionDebounceTicks;  // Debounce is not used for touch actions
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

    if (keysDown() & KEY_X) {
        show_ui = !show_ui;
        changed_screen = true;
    }

    if (keysDown() & KEY_SELECT) {
        tags_menu::open();
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
    if (changed_selection || changed_screen) {
        draw_top();
        draw_bottom();
    }

    changed_screen = false;
}

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

            if (screenshots::get_info(i).has_thumbnail) {
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
    set_target_screen(TargetScreen::BOTTOM);
    clear_target_screen(TargetScreen::BOTTOM);

    if (show_ui)
        draw_interface();
    else
        C2D_DrawImageAt(selected_screenshot.bottom, 0, 0, 0);
}

void draw_top() {
    if (!set_target_screen(TargetScreen::TOP)) return;

    clear_target_screen(TargetScreen::TOP);
    gfxSet3D(selected_screenshot.is_3d);

    C2D_DrawImageAt(selected_screenshot.top, 0, 0, 0);

    if (selected_screenshot.is_3d) {
        set_target_screen(TargetScreen::TOP_RIGHT);
        clear_target_screen(TargetScreen::TOP_RIGHT);

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
}  // namespace ui::viewer