
#include "ui/tag_editor.hpp"

#include <citro2d.h>

#include <array>
#include <iostream>
#include <numeric>
#include <vector>

#include "tags.hpp"
#include "ui/viewer.hpp"

namespace ui::tag_editor {

const int kPreviewTagPositionY = 6;
const int kTagHeight = 24;
const int kTagPadding = kTagHeight / 2;
const float kTagTextSize = 0.6;
const float kTagTextOffsetY = 3;
const int kMaxTagTextWidth = 288 - kTagPadding * 2;

const int kTabsPositionY = 37;
const int kNumTabs = 4;
const int kTabSpacing = 1;
const int kTabHeight = 24;
const float kTabTextSize = 0.6;
const float kTabTextOffsetY = 2;

const int kEditorAreaHeight = 153;
const int kEditorAreaPositionY = kTabsPositionY + kTabHeight + kTabSpacing;
const int kEditorMargin = 1;
const int kEditorButtonSpacing = 2;

const std::array<unsigned int, 3> kKeyboardNumKeysRows = {10, 9, 8};
const int kKeyboardRowOffset = 8;
const int kKeyboardPositionY = kEditorAreaPositionY + 3;
const int kKeyboardHeight = kEditorAreaHeight - 3;
const int kKeyboardLayoutKeyWidth = 62;
const int kKeyboardKeyWidth = kBottomScreenWidth / kKeyboardNumKeysRows[0] - kEditorButtonSpacing;
const int kKeyboardKeyHeight = kKeyboardHeight / 4 - kEditorButtonSpacing;
const float kKeyboardKeyTextSize = 0.6;
const float kKeyboardKeyTextOffsetY = 8;

const int kColorSelectorPositionY = kEditorAreaPositionY + 3;
const int kColorSelectorHeight = kEditorAreaHeight - 3;
const int kColorSelectorRows = 4;
const int kColorSelectorCols = 8;
const int kColorButtonWidth = kBottomScreenWidth / kColorSelectorCols - kEditorButtonSpacing;
const int kColorButtonHeight = kColorSelectorHeight / kColorSelectorRows - kEditorButtonSpacing;

const int kPosSelectorPositionY = kEditorAreaPositionY + 6;
const int kPosSelectorTagSpacing = 4;
const int kPosSelectorNumNeighborTags = 2;

const float kDeleteTagTextSize = 0.8;
const int kDeleteTagTextOffsetY = 80;
const int kDeleteTagButtonOffsetY = kDeleteTagTextOffsetY + 60;
const int kDeleteTagButtonWidth = 100;
const int kDeleteTagButtonHeight = 50;
const float kDeleteTagButtonTextSize = 0.75;
const int kDeleteTagButtonTextOffsetY = kDeleteTagButtonOffsetY + 12;

const int kNavbarHeight = 24;
const int kNavbarButtonsSpacing = 1;

const u32 clrTagPosHightlight = C2D_Color32(0x4F, 0x4F, 0x4F, 0xFF);

const std::vector<const char*> tab_titles = {"Label", "Color", "Position", "Delete"};

const std::array<const std::array<const std::string, 27>, 4> keyboard_keys = {
    {{"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Z", "X", "C", "V", "B", "N", "M", "Ç"},
     {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "a", "s", "d", "f", "g", "h", "j", "k", "l", "z", "x", "c", "v", "b", "n", "m", "ç"},
     {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\"", "!", "#", "~", "%", "&", "*", "(", "{", ".", ",", "/", ":", "<", "-", "+", "["},
     {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "'", "?", "@", "^", "$", "|", "°", ")", "}", "ª", "º", "\\", ";", ">", "_", "=", "]"}}};

const std::array<const std::array<std::tuple<const std::string, unsigned int>, 2>, 4> keyboard_layout_keys = {
    {{{{"Shift", 1}, {"123#!", 2}}}, {{{"Shift", 0}, {"123#!", 2}}}, {{{"1/2", 3}, {"ABC", 1}}}, {{{"2/2", 2}, {"ABC", 1}}}}};

const std::array<u32, 32> tag_colors = {
    0xff000000, 0xff532b1d, 0xff53257e, 0xff518700, 0xff3652ab, 0xff4f575f, 0xffc7c3c2, 0xffe8f1ff, 0xff4d00ff, 0xff00a3ff, 0xff27ecff,
    0xff36e400, 0xffffad29, 0xff9c7683, 0xffa877ff, 0xffaaccff, 0xff141829, 0xff351d11, 0xff362142, 0xff595312, 0xff292f74, 0xff3b3349,
    0xff7988a2, 0xff7deff3, 0xff5012be, 0xff246cff, 0xff2ee7a8, 0xff43b500, 0xffb55a06, 0xff654675, 0xff596eff, 0xff819dff,
};

tags::Tag edited_tag;
int edited_tag_index;

tags::tag_ptr original_tag;
int original_tag_index;

void (*return_edit_callback)(std::optional<tags::tag_ptr>);
void (*return_delete_callback)(tags::tag_ptr);
bool creating_new_tag;
bool changed;

unsigned int tab = 0;
unsigned int keyboard_layout = 0;

void Show(bool new_tag, tags::tag_ptr existing_tag, void (*edit_callback)(std::optional<tags::tag_ptr>), void (*delete_callback)(tags::tag_ptr)) {
    changed = true;
    SetUiFunctions(Input, Render);

    creating_new_tag = new_tag;
    original_tag = existing_tag;
    return_edit_callback = edit_callback;
    return_delete_callback = delete_callback;
    keyboard_layout = 0;
    tab = 0;

    if (creating_new_tag) {
        edited_tag = tags::Tag({"", clrWhite});
        edited_tag_index = tags::Count();
        original_tag_index = edited_tag_index;
    } else {
        edited_tag = *existing_tag;
        edited_tag_index = tags::GetTagIndex(existing_tag);
        original_tag_index = edited_tag_index;
    }
}

void FinishEditing(bool cancel) {
    std::optional<tags::tag_ptr> ret = {};
    if (!cancel) {
        if (creating_new_tag)
            ret = tags::AddTag(edited_tag);
        else
            tags::ReplaceTag(original_tag, edited_tag);

        if (original_tag_index != edited_tag_index) {
            tags::MoveTag(original_tag_index, edited_tag_index);
        }
    }
    return_edit_callback(ret);
}

void DeleteTag() {
    tags::DeleteTag(original_tag);
    return_delete_callback(original_tag);
}

// https://stackoverflow.com/a/37623867
void PopBackUTF8(std::string* utf8) {
    if (utf8->empty()) return;

    auto cp = utf8->data() + utf8->size();
    while (--cp >= utf8->data() && ((*cp & 0b10000000) && !(*cp & 0b01000000))) {
    }
    if (cp >= utf8->data()) utf8->resize(cp - utf8->data());
}

void TouchDownAction() {
    // Read the touch screen coordinates
    touchPosition touch;
    hidTouchRead(&touch);

    // Editor tabs
    float num_tabs = creating_new_tag ? kNumTabs - 1 : kNumTabs;
    float tab_width = kBottomScreenWidth / num_tabs - kTabSpacing;
    for (unsigned int i = 0; i < num_tabs; i++) {
        if (TouchedInRect(touch, i * (tab_width + kTabSpacing), kTabsPositionY, tab_width, kTabHeight)) {
            tab = i;
            changed = true;
        }
    }

    // Label tab
    if (tab == 0) {
        // Keyboard
        // Character key rows
        unsigned int current_key = 0;
        float key_x = 0, key_y = 0;
        for (unsigned int i = 0; i < kKeyboardNumKeysRows.size(); i++) {
            for (unsigned int j = 0; j < kKeyboardNumKeysRows[i]; j++) {
                key_x = kEditorMargin + i * kKeyboardRowOffset + j * (kKeyboardKeyWidth + kEditorButtonSpacing);
                key_y = kKeyboardPositionY + (kKeyboardKeyHeight + kEditorButtonSpacing) * i;
                if (TouchedInRect(touch, key_x, key_y, kKeyboardKeyWidth, kKeyboardKeyHeight)) {
                    edited_tag.name += keyboard_keys[keyboard_layout][current_key];
                    float w = GetTextWidth(kTagTextSize, edited_tag.name);
                    if (w > kMaxTagTextWidth) {
                        PopBackUTF8(&edited_tag.name);
                        return;
                    }

                    if (keyboard_layout == 0) {
                        keyboard_layout = 1;
                    }
                    changed = true;
                }
                current_key++;
            }
        }

        // Backspace button
        key_x += kEditorButtonSpacing + kKeyboardKeyWidth;
        const float backspace_width = kBottomScreenWidth - key_x - kEditorButtonSpacing;
        if (TouchedInRect(touch, key_x, key_y, backspace_width, kKeyboardKeyHeight)) {
            PopBackUTF8(&edited_tag.name);
            if (edited_tag.name.size() == 0 && keyboard_layout == 1) {
                keyboard_layout = 0;
            }
            changed = true;
        }

        // Layout key 0
        key_x = kEditorMargin;
        key_y += (kKeyboardKeyHeight + kEditorButtonSpacing);
        if (TouchedInRect(touch, key_x, key_y, kKeyboardLayoutKeyWidth, kKeyboardKeyHeight)) {
            keyboard_layout = std::get<1>(keyboard_layout_keys[keyboard_layout][0]);
            changed = true;
        }

        // Spacebar
        key_x += kKeyboardLayoutKeyWidth + kEditorButtonSpacing;
        const float spacebar_width = kBottomScreenWidth - key_x - kEditorButtonSpacing - kKeyboardLayoutKeyWidth - kEditorButtonSpacing;
        if (TouchedInRect(touch, key_x, key_y, spacebar_width, kKeyboardKeyHeight)) {
            edited_tag.name += " ";
            if (keyboard_layout == 1) {
                keyboard_layout = 0;
            }
            changed = true;
        }

        // Layout key 1
        key_x += spacebar_width + kEditorButtonSpacing;
        if (TouchedInRect(touch, key_x, key_y, kKeyboardLayoutKeyWidth, kKeyboardKeyHeight)) {
            keyboard_layout = std::get<1>(keyboard_layout_keys[keyboard_layout][1]);
            changed = true;
        }
    } else if (tab == 1) {
        for (unsigned int i = 0; i < kColorSelectorRows; i++) {
            float btn_y = kColorSelectorPositionY + (kColorButtonHeight + kEditorButtonSpacing) * i;
            for (unsigned int j = 0; j < kColorSelectorCols; j++) {
                unsigned int current_color = i * kColorSelectorCols + j;
                if (current_color >= tag_colors.size()) break;
                float btn_x = kEditorMargin + (kColorButtonWidth + kEditorButtonSpacing) * j;
                if (TouchedInRect(touch, btn_x, btn_y, kColorButtonWidth, kColorButtonHeight)) {
                    edited_tag.color = tag_colors[current_color];
                    changed = true;
                }
            }
        }
    } else if (tab == 2) {
        float editor_center_y = kEditorAreaPositionY + kEditorAreaHeight / 2;
        if (TouchedInRect(touch, 0, kEditorAreaPositionY, kBottomScreenWidth, editor_center_y - kEditorAreaPositionY)) {
            edited_tag_index = std::max(1, edited_tag_index) - 1;
            changed = true;
        }

        if (TouchedInRect(touch, 0, editor_center_y, kBottomScreenWidth, kEditorAreaHeight / 2)) {
            edited_tag_index = std::min(edited_tag_index + 1, static_cast<int>(tags::Count()) - (creating_new_tag ? 0 : 1));
            changed = true;
        }
    } else if (tab == 3) {
        if (TouchedInRect(touch, (kBottomScreenWidth - kDeleteTagButtonWidth) / 2, kDeleteTagButtonOffsetY, kDeleteTagButtonWidth, kDeleteTagButtonHeight)) {
            DeleteTag();
        }
    }

    if (TouchedInRect(touch, 0, kBottomScreenHeight - kNavbarHeight, kBottomScreenWidth / 2, kNavbarHeight)) {
        FinishEditing(true);
    }

    if (TouchedInRect(touch, kBottomScreenWidth / 2 + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight,
                      kBottomScreenWidth / 2 - kNavbarButtonsSpacing, kNavbarHeight)) {
        FinishEditing(false);
    }
}

void Input() {
    if ((keysDown() & KEY_B)) {
        FinishEditing(true);
    }

    if (keysDown() & KEY_TOUCH) {
        TouchDownAction();
    }

    if (tab == 2) {
        if (keysDown() & KEY_UP) {
            edited_tag_index = std::max(1, edited_tag_index) - 1;
            changed = true;
        }

        if (keysDown() & KEY_DDOWN) {
            edited_tag_index = std::min(edited_tag_index + 1, static_cast<int>(tags::Count()) - (creating_new_tag ? 0 : 1));
            changed = true;
        }
    }
}

void DrawTag(const tags::Tag* tag, float y) {
    std::string tag_text = tag->name == "" ? "Preview" : tag->name;
    u32 tag_text_color = tag->name == "" ? clrButtonsDisabled : clrBlack;
    float tag_width = GetTextWidth(kTagTextSize, tag_text) + kTagPadding * 2;
    float tag_x = (kBottomScreenWidth - tag_width) / 2;
    DrawRect(tag_x + kTagPadding, y, tag_width - kTagPadding * 2, kTagHeight, tag->color);
    DrawCircle(tag_x + kTagPadding, y + kTagHeight / 2, kTagHeight / 2, tag->color);
    DrawCircle(tag_x + tag_width - kTagPadding, y + kTagHeight / 2, kTagHeight / 2, tag->color);
    DrawText(tag_x + kTagPadding, y + kTagTextOffsetY, kTagTextSize, tag_text_color, tag_text, kLeft);
}

void Render(bool force) {
    if (!force && !changed) return;

    viewer::Render(true);

    // Background
    SetTargetScreen(TargetScreen::kBottom);
    DrawRect(0, 0, kBottomScreenWidth, kBottomScreenHeight, clrBackground);

    // Tag preview
    DrawTag(&edited_tag, kPreviewTagPositionY);

    // Editor tabs
    float num_tabs = creating_new_tag ? kNumTabs - 1 : kNumTabs;
    float tab_width = kBottomScreenWidth / num_tabs - kTabSpacing;
    for (unsigned int i = 0; i < num_tabs; i++) {
        DrawRect(i * (tab_width + kTabSpacing), kTabsPositionY, tab_width, kTabHeight + (tab == i ? kTabSpacing : 0),
                 tab == i ? clrButtonsDisabled : clrButtons);
        DrawText(i * tab_width + tab_width / 2, kTabsPositionY + kTabTextOffsetY, kTabTextSize, tab == i ? clrWhite : clrBlack, tab_titles[i]);
    }

    DrawRect(0, kEditorAreaPositionY, kBottomScreenWidth, kEditorAreaHeight, clrButtonsDisabled);

    // Label tab
    if (tab == 0) {
        // Keyboard
        // Character key rows
        unsigned int current_key = 0;
        float key_x = 0, key_y = 0;
        for (unsigned int i = 0; i < kKeyboardNumKeysRows.size(); i++) {
            for (unsigned int j = 0; j < kKeyboardNumKeysRows[i]; j++) {
                key_x = kEditorMargin + i * kKeyboardRowOffset + j * (kKeyboardKeyWidth + kEditorButtonSpacing);
                key_y = kKeyboardPositionY + (kKeyboardKeyHeight + kEditorButtonSpacing) * i;
                DrawRect(key_x, key_y, kKeyboardKeyWidth, kKeyboardKeyHeight, clrButtons);
                DrawText(key_x + kKeyboardKeyWidth / 2, key_y + kKeyboardKeyTextOffsetY, kKeyboardKeyTextSize, clrWhite,
                         keyboard_keys[keyboard_layout][current_key]);
                current_key++;
            }
        }

        // Backspace button
        key_x += kEditorButtonSpacing + kKeyboardKeyWidth;
        const float backspace_width = kBottomScreenWidth - key_x - kEditorMargin;
        DrawRect(key_x, key_y, backspace_width, kKeyboardKeyHeight, clrButtons);
        DrawText(key_x + backspace_width / 2, key_y + kKeyboardKeyTextOffsetY, kKeyboardKeyTextSize, clrWhite, "<-");

        // Layout key 0
        key_x = kEditorMargin;
        key_y += (kKeyboardKeyHeight + kEditorButtonSpacing);
        DrawRect(key_x, key_y, kKeyboardLayoutKeyWidth, kKeyboardKeyHeight, clrButtons);
        DrawText(key_x + kKeyboardLayoutKeyWidth / 2, key_y + kKeyboardKeyTextOffsetY, kKeyboardKeyTextSize, clrWhite,
                 std::get<0>(keyboard_layout_keys[keyboard_layout][0]));

        // Spacebar
        key_x += kKeyboardLayoutKeyWidth + kEditorButtonSpacing;
        const float spacebar_width = kBottomScreenWidth - key_x * 2;
        DrawRect(key_x, key_y, spacebar_width, kKeyboardKeyHeight, clrButtons);

        // Layout key 1
        key_x += spacebar_width + kEditorButtonSpacing;
        DrawRect(key_x, key_y, kKeyboardLayoutKeyWidth, kKeyboardKeyHeight, clrButtons);
        DrawText(key_x + kKeyboardLayoutKeyWidth / 2, key_y + kKeyboardKeyTextOffsetY, kKeyboardKeyTextSize, clrWhite,
                 std::get<0>(keyboard_layout_keys[keyboard_layout][1]));
    } else if (tab == 1) {
        for (unsigned int i = 0; i < kColorSelectorRows; i++) {
            float btn_y = kColorSelectorPositionY + (kColorButtonHeight + kEditorButtonSpacing) * i;
            for (unsigned int j = 0; j < kColorSelectorCols; j++) {
                unsigned int current_color = i * kColorSelectorCols + j;
                if (current_color >= tag_colors.size()) break;

                float btn_x = kEditorMargin + (kColorButtonWidth + kEditorButtonSpacing) * j;
                DrawRect(btn_x, btn_y, kColorButtonWidth, kColorButtonHeight, tag_colors[current_color]);
            }
        }
    } else if (tab == 2) {
        float editor_center_y = kEditorAreaPositionY + kEditorAreaHeight / 2;
        DrawRect(0, editor_center_y - kTagHeight / 2 - 2, kBottomScreenWidth, kTagHeight + 4, clrTagPosHightlight);
        DrawTag(&edited_tag, editor_center_y - kTagHeight / 2);

        float tag_y = editor_center_y - (kTagHeight + kPosSelectorTagSpacing) * kPosSelectorNumNeighborTags - kTagHeight / 2;
        int skip = !creating_new_tag && edited_tag_index - original_tag_index >= kPosSelectorNumNeighborTags ? 1 : 0;
        for (int i = -kPosSelectorNumNeighborTags; i < kPosSelectorNumNeighborTags; i++) {
            if (i == 0) {
                tag_y += (kTagHeight + kPosSelectorTagSpacing);
            }
            if (edited_tag_index + i == original_tag_index) {
                skip = 1;
            }
            if (edited_tag_index + (i + skip) >= 0 && edited_tag_index + (i + skip) < static_cast<int>(tags::Count())) {
                DrawTag(tags::Get(edited_tag_index + (i + skip)), tag_y);
            }
            tag_y += (kTagHeight + kPosSelectorTagSpacing);
        }
    } else if (tab == 3) {
        DrawText(kBottomScreenWidth / 2, kDeleteTagTextOffsetY, kDeleteTagTextSize, clrBlack, "Are you sure you want\n    to delete this tag?");
        DrawRect((kBottomScreenWidth - kDeleteTagButtonWidth) / 2, kDeleteTagButtonOffsetY, kDeleteTagButtonWidth, kDeleteTagButtonHeight, clrButtons);
        DrawText((kBottomScreenWidth) / 2, kDeleteTagButtonTextOffsetY, kDeleteTagButtonTextSize, clrWhite, "Delete");
    }

    if (tab != 3) {
        // Cancel
        DrawRect(0, kBottomScreenHeight - kNavbarHeight, kBottomScreenWidth / 2, kNavbarHeight, clrButtons);
        DrawText((kBottomScreenWidth / 2) / 2, kBottomScreenHeight - kNavbarHeight + kTabTextOffsetY, kTabTextSize, clrBlack, "Cancel");

        // Save
        DrawRect(kBottomScreenWidth / 2 + kNavbarButtonsSpacing, kBottomScreenHeight - kNavbarHeight, kBottomScreenWidth / 2 - kNavbarButtonsSpacing,
                 kNavbarHeight, clrButtons);
        DrawText(kBottomScreenWidth / 2 + kNavbarButtonsSpacing + (kBottomScreenWidth / 2) / 2, kBottomScreenHeight - kNavbarHeight + kTabTextOffsetY,
                 kTabTextSize, clrBlack, "Save changes");
    }

    // Top screen
    if (SetTargetScreen(TargetScreen::kTop)) {
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 38, 1, clrWhite, creating_new_tag ? "Create tag" : "Edit tag");

        SetTargetScreen(TargetScreen::kTopRight);
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 38, 1, clrWhite, creating_new_tag ? "Create tag" : "Edit tag");
    }

    changed = false;
}
}  // namespace ui::tag_editor
