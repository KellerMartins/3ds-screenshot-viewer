#include "ui/viewer.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <set>

#include "screenshots.hpp"
#include "settings.hpp"
#include "ui/tags_menu.hpp"

namespace ui::viewer {

const int kThumbnailSpacing = 4;
const int kNavbarHeight = 24;
const int kNavbarArrowWidth = 64;
const int kNavbarButtonsSpacing = 1;
const int kNavbarIconMargin = 24;
const int kNavbarIconSpacing = 4;
const int kNavbarIconScale = 16;
const int kNavbarHideButtonWidth = (kBottomScreenWidth - (kNavbarArrowWidth + kNavbarButtonsSpacing) * 2);

const int kNRows = ((kBottomScreenHeight - kNavbarHeight) / kThumbnailHeight);
const int kNCols = (kBottomScreenWidth / (kThumbnailWidth + kThumbnailSpacing));

const int kHMargin = (kBottomScreenWidth - kNCols * kThumbnailWidth - (kNCols - 1) * kThumbnailSpacing) / 2;
const int kVMargin = ((kBottomScreenHeight - kNavbarHeight) - kNRows * kThumbnailHeight - (kNRows - 1) * kThumbnailSpacing) / 2;

const int kSelectionOutline = 2;
const int kTagLineThickness = 4;

const unsigned int kSelectionDebounceTicks = 20;
const unsigned int kMultiSelectionTouchHoldTicks = 20;

const u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
const u32 clrButtons = C2D_Color32(0x7F, 0x7F, 0x7F, 0xFF);
const u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
const u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0xFF);

screenshots::Screenshot selected_screenshot;
std::set<std::string> multi_selection_screenshots;
std::set<tags::tag_ptr> tags_filter;
unsigned int selected_index = 0;
unsigned int page_index = 0;

int last_loaded_thumbs = 0;
unsigned int ticks_touch_held = 0;
unsigned int ticks_since_last_input = kSelectionDebounceTicks;

bool show_ui = true;
bool is_3d = false;
bool multi_selection_mode = false;
bool changed_selection = true;
bool changed_screen = true;
float slider = 0;

unsigned int GetPageIndex(int x) { return x / (kNRows * kNCols); }
unsigned int GetLastPageIndex() { return GetPageIndex(screenshots::Size() - 1); }
void DrawBottom();
void DrawTop();
void TouchDownActions();
void TouchHoldActions();
void ToggleScreenshotSelection(unsigned int);
void OnSelectScreenshotTags(bool, std::set<tags::tag_ptr>);
void OnSelectFilterTags(bool, std::set<tags::tag_ptr>);

void Show() {
    changed_screen = true;
    SetUiFunctions(Input, Render);
}

void Input() {
    if (!keysDown()) {
        ticks_since_last_input = ticks_since_last_input < kSelectionDebounceTicks ? ticks_since_last_input + 1 : kSelectionDebounceTicks;
    } else {
        ticks_since_last_input = 0;
    }

    if (keysDown() & KEY_TOUCH) {
        TouchDownActions();
        ticks_since_last_input = kSelectionDebounceTicks;  // Debounce is not used for touch actions
    }

    if (keysHeld() & KEY_TOUCH) {
        if (keysDown() & KEY_TOUCH)
            ticks_touch_held = 0;
        else
            ticks_touch_held = std::min(kMultiSelectionTouchHoldTicks, ticks_touch_held + 1);

        TouchHoldActions();
    }

    if (keysDown() & KEY_DLEFT) {
        selected_index = std::max(static_cast<unsigned int>(1), selected_index) - 1;
        page_index = GetPageIndex(selected_index);
        changed_selection = true;
    }

    if (keysDown() & KEY_DRIGHT) {
        selected_index = std::min(screenshots::Size() - 1, static_cast<size_t>(selected_index) + 1);
        page_index = GetPageIndex(selected_index);
        changed_selection = true;
    }

    if (keysDown() & KEY_DUP && selected_index - kNCols >= 0) {
        selected_index -= kNCols;
        page_index = GetPageIndex(selected_index);
        changed_selection = true;
    }

    if (keysDown() & KEY_DDOWN && selected_index + kNCols < screenshots::Size()) {
        selected_index += kNCols;
        page_index = GetPageIndex(selected_index);
        changed_selection = true;
    }

    if (keysDown() & KEY_L && page_index > 0) {
        page_index--;
        changed_screen = true;
    }

    if (keysDown() & KEY_R && page_index < GetLastPageIndex()) {
        page_index++;
        changed_screen = true;
    }

    if (keysDown() & KEY_A) {
        if (multi_selection_mode) {
            ToggleScreenshotSelection(selected_index);
        } else {
            show_ui = !show_ui;
        }
        changed_screen = true;
    }

    if (keysDown() & KEY_B) {
        if (multi_selection_mode) {
            multi_selection_mode = false;
        }
        changed_screen = true;
    }

    if (keysDown() & KEY_SELECT || keysDown() & KEY_X) {
        if (multi_selection_mode) {
            tags_menu::Show("Select screenshot tags", tags::GetScreenshotsTags(multi_selection_screenshots), OnSelectScreenshotTags);
        } else {
            tags_menu::Show("Filter by tags", tags_filter, OnSelectFilterTags);
        }
    }

    float new_slider = 1.0f - osGet3DSliderState();
    if (slider != new_slider) {
        slider = new_slider;
        changed_screen = true;
    }

    if (last_loaded_thumbs != screenshots::NumLoadedThumbnails()) {
        last_loaded_thumbs = screenshots::NumLoadedThumbnails();
        if (show_ui) changed_screen = true;
    }

    if (changed_selection && ticks_since_last_input >= kSelectionDebounceTicks) {
        selected_screenshot = screenshots::Load(selected_index);
        changed_selection = false;
        changed_screen = true;
        ticks_since_last_input = 0;
    }
}

void Render(bool force) {
    if (force || changed_selection || changed_screen) {
        DrawTop();
        DrawBottom();
    }

    changed_screen = false;
}

void DrawDownArrow(unsigned int x, unsigned int y, unsigned int scale) {
    C2D_DrawTriangle(x + scale / 2, y - scale / 4, clrBlack, x - scale / 2 - 0.5, y - scale / 4, clrBlack, x, y + scale / 4, clrBlack, 0);
}

void DrawInterface() {
    unsigned int i = page_index * (kNRows * kNCols);
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::Size()) break;

            if (selected_index == i) {
                DrawRect(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c - kSelectionOutline,
                         kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r - kSelectionOutline, kThumbnailWidth + kSelectionOutline * 2,
                         kThumbnailHeight + kSelectionOutline * 2, clrWhite);
            }

            screenshots::ScreenshotInfo screenshot = screenshots::GetInfo(i);
            if (screenshot.has_thumbnail) {
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, clrBlack, !multi_selection_mode || multi_selection_screenshots.contains(screenshot.name) ? 0 : 0.5);
                C2D_DrawImageAt(screenshot.thumbnail, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c,
                                kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, 0, &tint);
            } else {
                DrawRect(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, kThumbnailWidth,
                         kThumbnailHeight, clrButtons);
            }

            if (multi_selection_mode) {
                size_t num_tags = screenshot.tags.size();
                int tag_x = kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c;
                for (auto tag : screenshot.tags) {
                    DrawRect(tag_x, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r + kThumbnailHeight - kTagLineThickness, kThumbnailWidth / num_tags,
                             kTagLineThickness, tag->color);
                    tag_x += kThumbnailWidth / num_tags;
                }
            }

            i++;
        }
    }

    DrawRect(0, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight, clrButtons);
    DrawRect(kBottomScreenWidth - kNavbarArrowWidth, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight, clrButtons);
    DrawRect(kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kNavbarHideButtonWidth, kNavbarHeight, clrButtons);

    DrawLeftArrow(kNavbarIconMargin + kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);
    DrawRightArrow(kBottomScreenWidth - kNavbarIconMargin - kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);

    if (multi_selection_mode) {
        DrawText(kBottomScreenWidth / 2, kBottomScreenHeight - kNavbarHeight * 0.85, 0.5, clrBlack, "Back");
    } else {
        DrawDownArrow(kBottomScreenWidth / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);
    }
}

void DrawBottom() {
    SetTargetScreen(TargetScreen::kBottom);
    ClearTargetScreen(TargetScreen::kBottom);

    if (show_ui)
        DrawInterface();
    else
        C2D_DrawImageAt(selected_screenshot.bottom, 0, 0, 0);
}

void DrawTop() {
    if (!SetTargetScreen(TargetScreen::kTop)) return;

    ClearTargetScreen(TargetScreen::kTop, clrBlack);
    gfxSet3D(selected_screenshot.is_3d);

    C2D_DrawImageAt(selected_screenshot.top, selected_screenshot.is_3d ? -slider * settings::ExtraStereoOffset() : 0, 0, 0);

    if (selected_screenshot.is_3d) {
        SetTargetScreen(TargetScreen::kTopRight);
        ClearTargetScreen(TargetScreen::kTopRight, clrBlack);

        C2D_DrawImageAt(selected_screenshot.top_right, slider * settings::ExtraStereoOffset(), 0, 0);
    }
}

void TouchDownActions() {
    if (show_ui == false) {
        show_ui = true;
        changed_screen = true;
        return;
    }

    // Read the touch screen coordinates
    touchPosition touch;
    hidTouchRead(&touch);

    // Next page
    if (TouchedInRect(touch, kBottomScreenWidth - kNavbarArrowWidth, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight)) {
        page_index = std::min(GetLastPageIndex(), page_index + 1);
        changed_screen = true;
    }

    // Previous page
    if (TouchedInRect(touch, 0, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight)) {
        page_index = std::max(static_cast<unsigned int>(1), page_index) - 1;
        changed_screen = true;
    }

    // Hide UI / Cancel multi selection
    if (TouchedInRect(touch, kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kNavbarHideButtonWidth, kNavbarHeight)) {
        if (multi_selection_mode) {
            multi_selection_mode = false;
        } else {
            show_ui = false;
        }

        changed_screen = true;
    }

    // Select screenshot
    unsigned int i = page_index * (kNRows * kNCols);
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::Size()) break;

            if (TouchedInRect(touch, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r,
                              kThumbnailWidth, kThumbnailHeight)) {
                if (multi_selection_mode) {
                    ToggleScreenshotSelection(i);
                }

                selected_index = i;
                changed_selection = true;
                return;
            }
            i++;
        }
    }
}
void TouchHoldActions() {
    // Read the touch screen coordinates
    touchPosition touch;
    hidTouchRead(&touch);

    // Enter multi selection mode
    unsigned int i = page_index * (kNRows * kNCols);
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= screenshots::Size()) break;

            if (TouchedInRect(touch, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r,
                              kThumbnailWidth, kThumbnailHeight)) {
                if (ticks_touch_held == kMultiSelectionTouchHoldTicks && !multi_selection_mode) {
                    multi_selection_mode = true;
                    multi_selection_screenshots.clear();
                    ToggleScreenshotSelection(i);
                    changed_selection = true;
                }

                return;
            }
            i++;
        }
    }
}

void ToggleScreenshotSelection(unsigned int index) {
    std::string name = screenshots::GetInfo(index).name;
    if (multi_selection_screenshots.contains(name)) {
        multi_selection_screenshots.erase(multi_selection_screenshots.find(name));
    } else {
        multi_selection_screenshots.insert(name);
    }
}

void OnSelectScreenshotTags(bool changed_initial_selection, std::set<tags::tag_ptr> tags) {
    if (changed_initial_selection) {
        tags::SetScreenshotsTags(multi_selection_screenshots, tags);
    }
    Show();
}

void OnSelectFilterTags(bool changed_initial_selection, std::set<tags::tag_ptr> tags) { Show(); }
}  // namespace ui::viewer
