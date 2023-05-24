#include "ui/viewer.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
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

const unsigned int kInputDebounceTicks = 20;
const unsigned int kInputHoldTicks = 20;

const u32 clrScreenshotOverlay = C2D_Color32(0x11, 0x11, 0x11, 0x9F);

std::vector<size_t> filtered_screenshots;

screenshots::Screenshot selected_screenshot;
std::set<std::string> multi_selection_screenshots;
unsigned int selected_index = 0;
unsigned int page_index = 0;

size_t last_loaded_thumbs = 0;
unsigned int ticks_touch_held = 0;
unsigned int ticks_a_held = 0;
unsigned int ticks_since_last_input;

bool show_ui = true;
bool multi_selection_mode = false;
bool hide_last_image;  // Hides last screenshot before the next is loaded
bool changed_selection;
bool changed_screen;
float slider = 0;

unsigned int GetPageIndex(int x) { return x / (kNRows * kNCols); }
unsigned int GetLastPageIndex() { return GetPageIndex(filtered_screenshots.size() - 1); }

void DrawBottom();
void DrawTop();

void OnSelectScreenshotTags(bool, std::set<tags::tag_ptr>);
void OnSelectFilterTags(bool, std::set<tags::tag_ptr>);
void OnSelectHideTags(bool, std::set<tags::tag_ptr>);
void OpenSetTagsMenu();
void OpenHideTagsMenu();
void OpenFilterTagsMenu();
void ToggleScreenshotSelection(unsigned int);

void Show() {
    changed_screen = true;
    changed_selection = true;
    ticks_since_last_input = kInputDebounceTicks;
    SetUiFunctions(Input, Render);

    filtered_screenshots.clear();
    for (size_t i = 0; i < screenshots::Count(); i++) {
        screenshots::ScreenshotInfo screenshot = screenshots::GetInfo(i);
        if ((tags::GetTagsFilter().size() == 0 || screenshot.has_all_tag(tags::GetTagsFilter())) && !screenshot.has_any_tag(tags::GetHiddenTags()))
            filtered_screenshots.push_back(i);
    }
}

// Input Functions

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

    // Open tags menu
    if (TouchedInRect(touch, kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kNavbarHideButtonWidth, kNavbarHeight)) {
        OpenSetTagsMenu();
    }

    // Select screenshot
    size_t i = page_index * (kNRows * kNCols);
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= filtered_screenshots.size()) break;

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
            if (i >= filtered_screenshots.size()) break;

            if (TouchedInRect(touch, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r,
                              kThumbnailWidth, kThumbnailHeight)) {
                if (ticks_touch_held == kInputHoldTicks && !multi_selection_mode) {
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

void Input() {
    if (!keysDown()) {
        ticks_since_last_input = ticks_since_last_input < kInputDebounceTicks ? ticks_since_last_input + 1 : kInputDebounceTicks;
    } else {
        ticks_since_last_input = 0;
    }

    if (keysDown() & KEY_TOUCH) {
        TouchDownActions();
        ticks_since_last_input = kInputDebounceTicks;  // Debounce is not used for touch actions
    }

    if (keysHeld() & KEY_TOUCH) {
        if (keysDown() & KEY_TOUCH)
            ticks_touch_held = 0;
        else
            ticks_touch_held = std::min(kInputHoldTicks, ticks_touch_held + 1);

        TouchHoldActions();
    }

    if (keysDown() & KEY_DLEFT) {
        if (page_index == GetPageIndex(selected_index)) {
            selected_index = std::max(static_cast<unsigned int>(1), selected_index) - 1;
            page_index = GetPageIndex(selected_index);
        } else {
            selected_index = page_index * kNRows * kNCols;
        }
        changed_selection = true;
    }

    if (keysDown() & KEY_DRIGHT) {
        if (page_index == GetPageIndex(selected_index)) {
            selected_index = selected_index + 1;
            page_index = GetPageIndex(selected_index);
        } else {
            selected_index = ((page_index + 1) * kNRows * kNCols) - 1;
        }

        selected_index = std::min(filtered_screenshots.size() - 1, static_cast<size_t>(selected_index));
        changed_selection = true;
    }

    if (keysDown() & KEY_DUP && selected_index - kNCols >= 0) {
        if (page_index == GetPageIndex(selected_index)) {
            selected_index -= kNCols;
            page_index = GetPageIndex(selected_index);
        } else {
            selected_index = page_index * kNRows * kNCols;
        }
        changed_selection = true;
    }

    if (keysDown() & KEY_DDOWN && selected_index + kNCols < filtered_screenshots.size()) {
        if (page_index == GetPageIndex(selected_index)) {
            selected_index += kNCols;
            page_index = GetPageIndex(selected_index);
        } else {
            selected_index = ((page_index + 1) * kNRows * kNCols) - 1;
        }
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

    if (keysDown() & KEY_B) {
        if (multi_selection_mode) {
            multi_selection_mode = false;
        }
        changed_screen = true;
    }

    if (keysDown() & KEY_Y) {
        OpenFilterTagsMenu();
    }

    if (keysDown() & KEY_X) {
        OpenHideTagsMenu();
    }

    if (keysUp() & KEY_A && ticks_a_held != kInputHoldTicks) {
        if (multi_selection_mode) {
            ToggleScreenshotSelection(selected_index);
        } else {
            show_ui = !show_ui;
        }
        changed_screen = true;
    }

    if (keysHeld() & KEY_A) {
        if (keysDown() & KEY_A || !show_ui)
            ticks_a_held = 0;
        else
            ticks_a_held = std::min(kInputHoldTicks, ticks_a_held + 1);

        if (ticks_a_held == kInputHoldTicks && !multi_selection_mode) {
            multi_selection_mode = true;
            multi_selection_screenshots.clear();
            ToggleScreenshotSelection(selected_index);
            changed_selection = true;
        }
    }

    float new_slider = 1.0f - osGet3DSliderState();
    if (slider != new_slider) {
        slider = new_slider;
        changed_screen = true;
    }

    size_t new_loading_thumbs = screenshots::NumLoadedThumbnails();
    if (last_loaded_thumbs != new_loading_thumbs) {
        last_loaded_thumbs = new_loading_thumbs;
        if (show_ui) changed_screen = true;
    }

    if (changed_selection && ticks_since_last_input >= kInputDebounceTicks) {
        selected_screenshot = screenshots::Load(filtered_screenshots[selected_index]);
        hide_last_image = false;
        changed_selection = false;
        changed_screen = true;
        ticks_since_last_input = 0;
    }
}

// Drawing Functions

void Render(bool force) {
    if (force || changed_selection || changed_screen) {
        DrawTop();
        DrawBottom();
    }

    changed_screen = false;
}

void DrawInterface() {
    size_t i = page_index * (kNRows * kNCols);
    for (int r = 0; r < kNRows; r++) {
        for (int c = 0; c < kNCols; c++) {
            if (i >= filtered_screenshots.size()) break;

            screenshots::ScreenshotInfo screenshot = screenshots::GetInfo(filtered_screenshots[i]);

            bool is_selected_multi = multi_selection_screenshots.contains(screenshot.name);
            float offset = is_selected_multi ? kSelectionOutline : 0;

            if (screenshot.has_thumbnail) {
                float scale_x = is_selected_multi ? (kTopScreenWidth + kSelectionOutline * 8) / static_cast<float>(kTopScreenWidth) : 1;
                float scale_y = is_selected_multi ? (kTopScreenHeight + kSelectionOutline * 8) / static_cast<float>(kTopScreenHeight) : 1;

                // Draw screenshot thumbnail
                C2D_DrawImageAt(screenshot.thumbnail, kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c - offset,
                                kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r - offset, 0, nullptr, scale_x, scale_y);
            } else {
                // Draw placeholder rect while loading thumbnails
                DrawRect(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, kThumbnailWidth,
                         kThumbnailHeight, clrButtons);
            }

            if (multi_selection_mode) {
                // Draw tags
                size_t num_tags = screenshot.tags.size();
                int tag_x = kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c - offset;
                for (auto tag : screenshot.tags) {
                    DrawRect(tag_x, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r + kThumbnailHeight - kTagLineThickness + offset,
                             (kThumbnailWidth + offset * 2) / num_tags, kTagLineThickness, tag->color);
                    tag_x += (kThumbnailWidth + offset * 2) / num_tags;
                }

                // Draw dark overlay on deselected screenshots
                if (!is_selected_multi) {
                    DrawRect(kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c, kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r, kThumbnailWidth,
                             kThumbnailHeight, clrScreenshotOverlay);
                }
            }

            i++;
        }
    }

    // Draw selection outline
    if (GetPageIndex(selected_index) == page_index) {
        int r = (selected_index - page_index * kNRows * kNCols) / kNCols;
        int c = selected_index % kNCols;

        screenshots::ScreenshotInfo screenshot = screenshots::GetInfo(filtered_screenshots[selected_index]);

        bool is_selected_multi = multi_selection_screenshots.contains(screenshot.name);
        float offset = is_selected_multi ? kSelectionOutline : 0;
        float x = kHMargin + (kThumbnailWidth + kThumbnailSpacing) * c - kSelectionOutline - offset;
        float y = kVMargin + (kThumbnailHeight + kThumbnailSpacing) * r - kSelectionOutline - offset;
        float w = kThumbnailWidth + kSelectionOutline * 2 + offset * 2;
        float h = kThumbnailHeight + kSelectionOutline * 2 + offset * 2;

        u32 outlineColor = (is_selected_multi || !multi_selection_mode) ? clrWhite : clrButtons;

        DrawOutlineRect(x, y, w, h, kSelectionOutline, outlineColor);
    }

    // Draw navbar buttons
    DrawRect(0, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight, page_index > 0 ? clrButtons : clrButtonsDisabled);
    DrawRect(kBottomScreenWidth - kNavbarArrowWidth, kBottomScreenHeight - kNavbarHeight, kNavbarArrowWidth, kNavbarHeight,
             page_index < GetLastPageIndex() ? clrButtons : clrButtonsDisabled);
    DrawRect(kNavbarArrowWidth + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kNavbarHideButtonWidth, kNavbarHeight, clrButtons);

    DrawLeftArrow(kNavbarIconMargin + kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale,
                  page_index > 0 ? clrBlack : clrBackground);
    DrawRightArrow(kBottomScreenWidth - kNavbarIconMargin - kNavbarIconScale / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale,
                   page_index < GetLastPageIndex() ? clrBlack : clrBackground);
    DrawUpArrow(kBottomScreenWidth / 2, kBottomScreenHeight - kNavbarHeight / 2, kNavbarIconScale);
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
    gfxSet3D(selected_screenshot.is_3d && !hide_last_image);

    if (hide_last_image) {
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrBackground);
        return;
    }

    C2D_DrawImageAt(selected_screenshot.top, selected_screenshot.is_3d ? -slider * settings::ExtraStereoOffset() : 0, 0, 0);

    if (selected_screenshot.is_3d) {
        SetTargetScreen(TargetScreen::kTopRight);
        ClearTargetScreen(TargetScreen::kTopRight, clrBlack);

        C2D_DrawImageAt(selected_screenshot.top_right, slider * settings::ExtraStereoOffset(), 0, 0);
    }
}

// Misc. functions

void OnSelectScreenshotTags(bool changed_initial_selection, std::set<tags::tag_ptr> tags) {
    if (changed_initial_selection) {
        tags::SetScreenshotsTags(multi_selection_screenshots, tags);
        multi_selection_screenshots.clear();
    }
    Show();
}

void OnSelectFilterTags(bool changed_initial_selection, std::set<tags::tag_ptr> tags) {
    if (changed_initial_selection) {
        tags::SetTagsFilter(tags);
        selected_index = 0;
        page_index = 0;
        hide_last_image = true;
        multi_selection_screenshots.clear();
    }
    Show();
}

void OnSelectHideTags(bool changed_initial_selection, std::set<tags::tag_ptr> tags) {
    if (changed_initial_selection) {
        tags::SetHiddenTags(tags);
        selected_index = 0;
        page_index = 0;
        hide_last_image = true;
        multi_selection_screenshots.clear();
    }
    Show();
}

void OpenSetTagsMenu() {
    if (!multi_selection_mode) {
        multi_selection_screenshots = std::set<std::string>{screenshots::GetInfo(filtered_screenshots[selected_index]).name};
    }
    tags_menu::Show("Set screenshot tags", true, tags::GetScreenshotsTags(multi_selection_screenshots), OnSelectScreenshotTags);
}

void OpenHideTagsMenu() { tags_menu::Show("Hide with tags", false, tags::GetHiddenTags(), OnSelectHideTags); }

void OpenFilterTagsMenu() { tags_menu::Show("Filter by tags", false, tags::GetTagsFilter(), OnSelectFilterTags); }

void ToggleScreenshotSelection(unsigned int index) {
    std::string name = screenshots::GetInfo(filtered_screenshots[index]).name;
    if (multi_selection_screenshots.contains(name)) {
        multi_selection_screenshots.erase(multi_selection_screenshots.find(name));
    } else {
        multi_selection_screenshots.insert(name);
    }
}
}  // namespace ui::viewer
