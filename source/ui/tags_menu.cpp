
#include "ui/tags_menu.hpp"

#include <citro2d.h>

#include <numeric>
#include <vector>

#include "tags.hpp"
#include "ui/tag_editor.hpp"
#include "ui/viewer.hpp"

namespace ui::tags_menu {

const int kMenuWidth = 300;
const int kMenuMaxHeight = 170;
const int kMenuMinRows = 3;
const int kMenuPositionY = kBottomScreenHeight - kMenuMaxHeight;
const int kMenuBorderRadius = 15;

const int kButtonHeight = 24;
const int kButtonSpacing = 1;
const int kButtonTextOffsetY = -7;
const int kButtonArrowWidth = 64;
const int kButtonArrowSize = 16;

const int kTagsMaxPositionY = kMenuPositionY;
const int kTagHeight = 24;
const int kTagPadding = kTagHeight / 2;
const int kTagMargin = 3;
const int kTagRows = 5;
const int kTagRowOffsetY = kTagHeight + kTagMargin;
const float kTagTextSize = 0.6;
const float kTagTextOffsetY = 3;

const unsigned int kTagEditTouchHoldTicks = 20;

struct TagItem {
    tags::tag_ptr tag;
    int index;
    float width;
    bool selected;
    TagItem(tags::tag_ptr tag, int index, bool selected)
        : tag(tag), index(index), width(GetTextWidth(kTagTextSize, tag->name) + kTagPadding * 2), selected(selected) {}
};

struct TagRow {
    std::vector<TagItem> items;
    float width = 0;

    TagItem* touched(float y, touchPosition touch) {
        float x = this->x();
        for (TagItem& item : items) {
            if (TouchedInRect(touch, x, y, item.width, kTagHeight)) {
                return &item;
            }

            x += item.width + kTagMargin;
        }

        return nullptr;
    }

    void draw(float y) {
        float x = this->x();
        for (TagItem& item : items) {
            u32 tag_color = item.selected ? item.tag->color : clrButtons;
            u32 text_color = item.selected ? (GetApproximateColorBrightness(item.tag->color) >= kContrastThreshold ? clrBlack : clrWhite) : clrBlack;

            DrawRect(x + kTagPadding, y, item.width - kTagPadding * 2, kTagHeight, tag_color);
            DrawCircle(x + kTagPadding, y + kTagHeight / 2, kTagHeight / 2, tag_color);
            DrawCircle(x + item.width - kTagPadding, y + kTagHeight / 2, kTagHeight / 2, tag_color);
            DrawText(x + kTagPadding, y + kTagTextOffsetY, kTagTextSize, text_color, item.tag->name, kLeft);
            x += item.width + kTagMargin;
        }
    }

    float x() {
        return (kBottomScreenWidth - kMenuWidth) / 2  // Border of the menu
               + (kMenuWidth - width) / 2;            // Margin to center tags
    }
};

std::string top_title;
std::vector<TagRow> tag_rows;
bool can_create_tags;
void (*return_callback)(bool, std::set<tags::tag_ptr>);

unsigned int row_offset = 0;
unsigned int ticks_touch_held = 0;
float reduced_menu_height = 0;
float reduced_menu_tags_offset = 0;

touchPosition touch;
bool touched_down;
bool changed_initial_selection;
bool changed;

void Show(std::string title, bool allow_create_tag, std::set<tags::tag_ptr> selected_tags, void (*callback)(bool, std::set<tags::tag_ptr>)) {
    changed = true;
    SetUiFunctions(Input, Render);

    top_title = title;
    can_create_tags = allow_create_tag;
    return_callback = callback;
    changed_initial_selection = false;
    touched_down = false;
    ticks_touch_held = 0;
    tag_rows = {TagRow()};

    for (int index = 0; index < static_cast<int>(tags::Count()); index++) {
        auto tag = tags::Get(index);
        TagItem item = TagItem(tag, index, selected_tags.contains(tag));

        if (tag_rows.back().width > 0 && tag_rows.back().width + item.width + kTagMargin > kMenuWidth) {
            tag_rows.push_back(TagRow());
        }

        tag_rows.back().items.push_back(item);
        tag_rows.back().width += item.width + kTagMargin;
    }

    int num_rows = tag_rows.size();
    reduced_menu_height = tag_rows.size() < kTagRows ? (kTagRows - std::max(kMenuMinRows, num_rows)) * (kTagHeight + kTagMargin) : 0;
    reduced_menu_tags_offset =
        reduced_menu_height + ((kMenuMaxHeight - reduced_menu_height - kButtonHeight) - std::min(num_rows, kTagRows) * (kTagHeight + kTagMargin)) / 2;

    while (row_offset >= tag_rows.size()) {
        if (row_offset >= kTagRows) {
            row_offset -= kTagRows;
        } else {
            row_offset = 0;
            break;
        }
    }
}

std::set<tags::tag_ptr> GetSelectedTags() {
    std::set<tags::tag_ptr> selected;

    for (TagRow& row : tag_rows) {
        for (TagItem& item : row.items) {
            if (item.selected) {
                selected.insert(item.tag);
            }
        }
    }

    return selected;
}

void Close() {
    std::set<tags::tag_ptr> selected;

    if (changed_initial_selection) {
        selected = GetSelectedTags();
    }

    return_callback(changed_initial_selection, selected);
}

void OnTagEdited(std::optional<tags::tag_ptr> new_tag) {
    auto selected_tags = GetSelectedTags();
    bool created_new_tag = new_tag.has_value();
    if (created_new_tag) {
        selected_tags.insert(new_tag.value());
    }

    Show(top_title, can_create_tags, selected_tags, return_callback);

    if (created_new_tag) {
        changed_initial_selection = true;
    }
}

void OnTagDeleted(tags::tag_ptr deleted_id) {
    auto selected_tags = GetSelectedTags();
    if (selected_tags.contains(deleted_id)) {
        selected_tags.erase(selected_tags.find(deleted_id));
    }

    Show(top_title, can_create_tags, selected_tags, return_callback);
}

void Input() {
    if ((keysDown() & KEY_SELECT) || (keysDown() & KEY_B) || (keysDown() & KEY_X) || (keysDown() & KEY_Y)) {
        Close();
    }

    if (keysDown() & KEY_DLEFT) {
        row_offset = std::max(0, static_cast<int>(row_offset) - kTagRows);
        changed = true;
    }

    if (keysDown() & KEY_DRIGHT) {
        row_offset = std::min(tag_rows.size() - 1, row_offset + kTagRows);
        changed = true;
    }

    // Read the touch screen coordinates
    if (keysDown() & KEY_TOUCH || keysHeld() & KEY_TOUCH) {
        hidTouchRead(&touch);
    }

    if (keysDown() & KEY_TOUCH) {
        touched_down = true;

        // Touched outside the menu
        if (!TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height), kMenuWidth,
                           (kMenuMaxHeight - reduced_menu_height))) {
            Close();
        }

        // Previous tags page
        if (TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                          kButtonArrowWidth, kButtonHeight)) {
            row_offset = std::max(0, static_cast<int>(row_offset) - kTagRows);
            changed = true;
        }

        // Next tags page
        if (row_offset + kTagRows < tag_rows.size() &&
            TouchedInRect(touch, kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonSpacing,
                          kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight)) {
            row_offset = std::min(tag_rows.size() - 1, row_offset + kTagRows);
            changed = true;
        }

        // Close menu
        if ((tag_rows.size() <= kTagRows &&
             TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                           kMenuWidth - kButtonSpacing * 2, kButtonHeight)) ||
            TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing * 2 + kButtonArrowWidth,
                          kBottomScreenHeight - kButtonHeight - kButtonSpacing, kMenuWidth - kButtonSpacing * 4 - kButtonArrowWidth * 2, kButtonHeight)) {
            Close();
        }
    }

    if (keysHeld() & KEY_TOUCH) {
        if (keysDown() & KEY_TOUCH)
            ticks_touch_held = 0;
        else
            ticks_touch_held = std::min(kTagEditTouchHoldTicks, ticks_touch_held + 1);

        if (ticks_touch_held == kTagEditTouchHoldTicks) {
            float y = kTagsMaxPositionY + reduced_menu_tags_offset;
            for (size_t i = row_offset; i < tag_rows.size() && i - row_offset < kTagRows; i++) {
                TagItem* touched_tag = tag_rows[i].touched(y, touch);
                if (touched_tag != nullptr) {
                    tag_editor::Show(false, touched_tag->tag, OnTagEdited, OnTagDeleted);
                    return;
                }
                y += kTagRowOffsetY;
            }

            // Touched and held in the empty area of the menu
            if (can_create_tags && TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height),
                                                 kMenuWidth, (kMenuMaxHeight - reduced_menu_height))) {
                tag_editor::Show(true, 0, OnTagEdited, OnTagDeleted);
            }
        }
    }

    if (keysUp() & KEY_TOUCH && touched_down) {
        touched_down = false;

        float y = kTagsMaxPositionY + reduced_menu_tags_offset;
        for (size_t i = row_offset; i < tag_rows.size() && i - row_offset < kTagRows; i++) {
            TagItem* touched_tag = tag_rows[i].touched(y, touch);
            if (touched_tag != nullptr) {
                touched_tag->selected = !touched_tag->selected;
                changed = true;
                changed_initial_selection = true;
                return;
            }
            y += kTagRowOffsetY;
        }
    }
}

void Render(bool force) {
    if (!force && !changed) return;

    viewer::Render(true);

    // Menu overlay
    SetTargetScreen(TargetScreen::kBottom);
    DrawRect(0, 0, kBottomScreenWidth, kBottomScreenHeight, clrOverlay);
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height) + kMenuBorderRadius, kMenuWidth,
             (kMenuMaxHeight - reduced_menu_height) - kMenuBorderRadius, clrBackground);
    DrawCircle((kBottomScreenWidth - kMenuWidth) / 2 + kMenuBorderRadius, kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height) + kMenuBorderRadius,
               kMenuBorderRadius, clrBackground);
    DrawCircle((kBottomScreenWidth - kMenuWidth) / 2 + kMenuWidth - kMenuBorderRadius,
               kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height) + kMenuBorderRadius, kMenuBorderRadius, clrBackground);
    DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kMenuBorderRadius, kBottomScreenHeight - (kMenuMaxHeight - reduced_menu_height),
             kMenuWidth - kMenuBorderRadius * 2, kMenuBorderRadius, clrBackground);

    // Tags
    float y = kTagsMaxPositionY + reduced_menu_tags_offset;
    for (size_t i = row_offset; i < tag_rows.size() && i - row_offset < kTagRows; i++) {
        tag_rows[i].draw(y);
        y += kTagRowOffsetY;
    }

    if (tag_rows.size() <= kTagRows) {
        // Close menu button
        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing, kMenuWidth - kButtonSpacing * 2,
                 kButtonHeight, clrButtons);
        DrawDownArrow(kBottomScreenWidth / 2, kBottomScreenHeight - kButtonHeight / 2, kButtonArrowSize);

        if (tags::Count() == 0 && can_create_tags) {
            DrawText(kBottomScreenWidth / 2, kBottomScreenHeight / 2 + 29, 0.8, clrButtons, "Touch and hold here");
            DrawText(kBottomScreenWidth / 2, kBottomScreenHeight / 2 + 47, 0.8, clrButtons, "to create a tag");
        }
    } else {
        // Navigation arrows
        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight,
                 row_offset > 0 ? clrButtons : clrButtonsDisabled);
        DrawRect(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonSpacing,
                 kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight,
                 row_offset + kTagRows < tag_rows.size() ? clrButtons : clrButtonsDisabled);

        DrawLeftArrow((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing + kButtonArrowWidth / 2, kBottomScreenHeight - kButtonHeight / 2, kButtonArrowSize,
                      row_offset > 0 ? clrBlack : clrBackground);
        DrawRightArrow(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth / 2, kBottomScreenHeight - kButtonHeight / 2,
                       kButtonArrowSize, row_offset + kTagRows < tag_rows.size() ? clrBlack : clrBackground);

        // Close menu button
        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing * 2 + kButtonArrowWidth, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                 kMenuWidth - kButtonSpacing * 4 - kButtonArrowWidth * 2, kButtonHeight, clrButtons);
        DrawDownArrow(kBottomScreenWidth / 2, kBottomScreenHeight - kButtonHeight / 2, kButtonArrowSize);
    }

    if (SetTargetScreen(TargetScreen::kTop)) {
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, top_title);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 15, 0.4, clrWhite,
                 can_create_tags ? "Touch and hold the menu to create a tag, touch and hold a tag to edit" : "Touch and hold a tag to edit");

        SetTargetScreen(TargetScreen::kTopRight);
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, top_title);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 15, 0.4, clrWhite,
                 can_create_tags ? "Touch and hold the menu to create a tag, touch and hold a tag to edit" : "Touch and hold a tag to edit");
    }

    changed = false;
}
}  // namespace ui::tags_menu
