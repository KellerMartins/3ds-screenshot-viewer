
#include "ui/settings_menu.hpp"

#include <citro2d.h>

#include <iostream>
#include <numeric>
#include <vector>

#include "screenshots.hpp"
#include "settings.hpp"
#include "ui/viewer.hpp"

namespace ui::settings_menu {

const int kMenuWidth = 250;
const int kMenuHeight = 210;
const int kMenuPositionY = kBottomScreenHeight - kMenuHeight;
const int kMenuBorderRadius = 6;

const int kButtonHeight = 24;
const int kButtonMargin = 12;
const int kButtonSpacing = 1;
const int kButtonTextOffsetY = 2;
const int kButtonArrowWidth = 36;
const int kButtonArrowSize = 16;

const int kOptionButtonHeight = 48;
const int kOptionPositionY = kMenuPositionY + kButtonMargin;
const int kOptionTextMargin = 24;
const int kOptionMargin = kOptionButtonHeight + 12;
const float kOptionTextSize = 0.6;

const int kDeleteSelectedButtonHeight = 24;
const int kDeleteSelectedButtonTextOffsetY = 3;
const float kDeleteSelectedButtonTextSize = 0.6;

const int kMinExtra3DOffset = 0;
const int kMaxExtra3DOffset = 10;

unsigned int row_offset = 0;
unsigned int ticks_touch_held = 0;
float reduced_menu_height = 0;
float reduced_menu_tags_offset = 0;

void (*return_callback)();

touchPosition touch;
bool changed_initial_options;
bool changed;

void Show(void (*callback)()) {
    changed = true;
    changed_initial_options = false;
    SetUiFunctions(Input, Render);

    return_callback = callback;
}

void Close() {
    if (changed_initial_options) {
        settings::Save();
    }
    return_callback();
}

int TouchedOption(touchPosition touch, float y) {
    // Navigation arrows
    if (TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonMargin, y, kButtonArrowWidth, kOptionButtonHeight)) {
        return -1;
    }
    if (TouchedInRect(touch, kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonMargin, y, kButtonArrowWidth,
                      kOptionButtonHeight)) {
        return 1;
    }

    return 0;
}

void Input() {
    if ((keysDown() & KEY_SELECT) || (keysDown() & KEY_B)) {
        Close();
    }

    // Read the touch screen coordinates
    if (keysDown() & KEY_TOUCH || keysHeld() & KEY_TOUCH) {
        hidTouchRead(&touch);
    }

    if (keysDown() & KEY_TOUCH) {
        // Touched outside the menu
        if (!TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - kMenuHeight, kMenuWidth, kMenuHeight)) {
            Close();
        }

        // Close menu
        if (TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                          kMenuWidth - kButtonSpacing * 2, kButtonHeight)) {
            Close();
        }

        float y = kOptionPositionY;
        int touchedSort = TouchedOption(touch, y);
        if (touchedSort != 0) {
            auto current_order = screenshots::GetOrder();

            auto new_order = static_cast<screenshots::ScreenshotOrder>(current_order + touchedSort);
            if (new_order <= screenshots::ScreenshotOrder::kLast && new_order >= screenshots::ScreenshotOrder::kFirst) {
                screenshots::SetOrder(new_order);
            }
            changed = true;
            changed_initial_options = true;
            return;
        }

        y += kOptionMargin;
        int touchedExtra3D = TouchedOption(touch, y);
        if (touchedExtra3D != 0) {
            auto current_offset = settings::GetExtraStereoOffset();
            auto new_offset = std::min(kMaxExtra3DOffset, std::max(kMinExtra3DOffset, current_offset + touchedExtra3D));
            settings::SetExtraStereoOffset(new_offset);
            changed = true;
            changed_initial_options = true;
            return;
        }
    }
}

void DrawOption(float y, std::string label, std::string value, bool enable_left, bool enable_right) {
    DrawText(kBottomScreenWidth / 2, y, kOptionTextSize, clrWhite, label);

    // Navigation arrows
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonMargin, y, kButtonArrowWidth, kOptionButtonHeight, enable_left ? clrButtons : clrButtonsDisabled);
    DrawRect(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonMargin, y, kButtonArrowWidth, kOptionButtonHeight,
             enable_right ? clrButtons : clrButtonsDisabled);

    DrawLeftArrow((kBottomScreenWidth - kMenuWidth) / 2 + kButtonMargin + kButtonArrowWidth / 2, y + kOptionButtonHeight / 2, kButtonArrowSize,
                  enable_left ? clrBlack : clrBackground);
    DrawRightArrow(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth / 2 - kButtonMargin, y + kOptionButtonHeight / 2,
                   kButtonArrowSize, enable_right ? clrBlack : clrBackground);

    DrawText(kBottomScreenWidth / 2, y + kOptionTextMargin + kButtonTextOffsetY, kOptionTextSize, clrButtons, value);
}

void Render(bool force) {
    if (!force && !changed) return;

    viewer::Render(true);

    // Menu overlay
    SetTargetScreen(TargetScreen::kBottom);
    DrawRect(0, 0, kBottomScreenWidth, kBottomScreenHeight, clrOverlay);
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - kMenuHeight + kMenuBorderRadius, kMenuWidth, kMenuHeight - kMenuBorderRadius,
             clrBackground);
    DrawCircle((kBottomScreenWidth - kMenuWidth) / 2 + kMenuBorderRadius, kBottomScreenHeight - kMenuHeight + kMenuBorderRadius, kMenuBorderRadius,
               clrBackground);
    DrawCircle((kBottomScreenWidth - kMenuWidth) / 2 + kMenuWidth - kMenuBorderRadius, kBottomScreenHeight - kMenuHeight + kMenuBorderRadius, kMenuBorderRadius,
               clrBackground);
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kMenuBorderRadius, kBottomScreenHeight - kMenuHeight, kMenuWidth - kMenuBorderRadius * 2,
             kMenuBorderRadius, clrBackground);

    // Close menu button
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing, kMenuWidth - kButtonSpacing * 2,
             kButtonHeight, clrButtons);
    DrawDownArrow(kBottomScreenWidth / 2, kBottomScreenHeight - kButtonHeight / 2, kButtonArrowSize);

    // Options
    float y = kOptionPositionY;
    screenshots::ScreenshotOrder order = screenshots::GetOrder();
    DrawOption(y, "Sort by",
               order == screenshots::ScreenshotOrder::kNewer       ? "Newer"
               : order == screenshots::ScreenshotOrder::kOlder     ? "Oldest"
               : order == screenshots::ScreenshotOrder::kTags      ? "Tags"
               : order == screenshots::ScreenshotOrder::kTagsNewer ? "Tags (newer first)"
                                                                   : "Unknown",
               order != screenshots::ScreenshotOrder::kFirst, order != screenshots::ScreenshotOrder::kLast);

    y += kOptionMargin;
    int offset_3D = settings::GetExtraStereoOffset();
    DrawOption(y, "Extra 3D offset", std::to_string(offset_3D), offset_3D != kMinExtra3DOffset, offset_3D != kMaxExtra3DOffset);

    y += kOptionMargin + 10;
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonMargin, y, kMenuWidth - kButtonMargin * 2, kDeleteSelectedButtonHeight, clrButtonsDisabled);
    DrawText((kBottomScreenWidth) / 2, y + kDeleteSelectedButtonTextOffsetY, kDeleteSelectedButtonTextSize, clrBackground, "Delete selected screenshots");

    if (SetTargetScreen(TargetScreen::kTop)) {
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, "Settings");

        SetTargetScreen(TargetScreen::kTopRight);
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, "Settings");
    }

    changed = false;
}
}  // namespace ui::settings_menu
