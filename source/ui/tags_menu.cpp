
#include "ui/tags_menu.hpp"

#include <citro2d.h>

#include <iostream>
#include <numeric>
#include <vector>

#include "tags.hpp"
#include "ui/viewer.hpp"

namespace ui::tags_menu {

const int kMenuWidth = 300;
const int kMenuHeight = 197;
const int kMenuPositionY = kBottomScreenHeight - kMenuHeight;
const int kMenuBorderRadius = 15;

const int kButtonHeight = 24;
const int kButtonSpacing = 1;
const int kButtonTextOffsetY = 3;
const int kButtonArrowWidth = 64;
const int kButtonArrowSize = 16;

const int kTagsPositionY = kMenuPositionY + 6;
const int kTagHeight = 25;
const int kTagPadding = kTagHeight / 2;
const int kTagMargin = 2;
const int kTagRows = 6;
const int kTagRowOffsetY = kTagHeight + kTagMargin;
const float kTagTextSize = 0.6;
const float kTagTextOffsetY = 3;

const u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
const u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
const u32 clrButtons = C2D_Color32(0x7F, 0x7F, 0x7F, 0xFF);
const u32 clrButtonsDisabled = C2D_Color32(0x5F, 0x5F, 0x5F, 0xFF);
const u32 clrBackground = C2D_Color32(0x40, 0x40, 0x40, 0xFF);
const u32 clrOverlay = C2D_Color32(0x11, 0x11, 0x11, 0x7F);

struct TagItem {
    tags::Tag* tag;
    float width;
    tags::TagId id;
    bool selected;
    TagItem(tags::Tag* tag, tags::TagId id, bool selected)
        : tag(tag), width(GetTextWidth(kTagTextSize, tag->name) + kTagPadding * 2), id(id), selected(selected) {}
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
            u32 text_color = item.selected ? clrWhite : clrBlack;

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
void (*return_callback)(bool, std::set<tags::TagId>);
unsigned int row_offset = 0;
bool changed_initial_selection;
bool changed;

void Show(std::string title, std::set<tags::TagId> selected_tags, void (*callback)(bool, std::set<tags::TagId>)) {
    changed = true;
    SetUiFunctions(Input, Render);

    top_title = title;
    return_callback = callback;
    changed_initial_selection = false;
    tag_rows = {TagRow()};

    for (tags::TagId id = 0; id < tags::Size(); id++) {
        TagItem item = TagItem(tags::Get(id), id, selected_tags.contains(id));

        if (tag_rows.back().width > 0 && tag_rows.back().width + item.width + kTagMargin > kMenuWidth) {
            tag_rows.push_back(TagRow());
        }

        tag_rows.back().items.push_back(item);
        tag_rows.back().width += item.width;
    }
}

void Close() {
    std::set<tags::TagId> selected;

    if (changed_initial_selection) {
        for (TagRow& row : tag_rows) {
            for (TagItem& item : row.items) {
                if (item.selected) {
                    selected.insert(item.id);
                }
            }
        }
    }

    return_callback(changed_initial_selection, selected);
}

void Input() {
    if ((keysDown() & KEY_SELECT) || (keysDown() & KEY_B) || (keysDown() & KEY_X)) {
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

    if (keysDown() & KEY_TOUCH) {
        // Read the touch screen coordinates
        touchPosition touch;
        hidTouchRead(&touch);

        // Touched inside the menu
        if (TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2, kBottomScreenHeight - kMenuHeight, kMenuWidth, kMenuHeight)) {
            float y = kTagsPositionY;
            for (size_t i = row_offset; i < tag_rows.size() && i - row_offset < kTagRows; i++) {
                TagItem* touched_tag = tag_rows[i].touched(y, touch);
                if (touched_tag != nullptr) {
                    touched_tag->selected = !touched_tag->selected;
                    changed = true;
                    changed_initial_selection = true;
                    break;
                }
                y += kTagRowOffsetY;
            }

            if (TouchedInRect(touch, (kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                              kButtonArrowWidth, kButtonHeight)) {
                row_offset = std::max(0, static_cast<int>(row_offset) - kTagRows);
                changed = true;
            }

            if (row_offset + kTagRows < tag_rows.size() &&
                TouchedInRect(touch, kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonSpacing,
                              kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight)) {
                row_offset = std::min(tag_rows.size() - 1, row_offset + kTagRows);
                changed = true;
            }
        } else {
            // Touched outside
            Close();
        }
    }
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

    // Tags
    float y = kTagsPositionY;
    for (size_t i = row_offset; i < tag_rows.size() && i - row_offset < kTagRows; i++) {
        tag_rows[i].draw(y);
        y += kTagRowOffsetY;
    }

    if (tag_rows.size() <= 7) {
        // Action button
        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing, kMenuWidth - kButtonSpacing * 2,
                 kButtonHeight, clrButtons);
        DrawText(kBottomScreenWidth / 2, kBottomScreenHeight - kButtonHeight + kButtonTextOffsetY, 0.5, clrBlack, "Add Tag");
    } else {
        // Action button
        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing, kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight,
                 row_offset > 0 ? clrButtons : clrButtonsDisabled);
        DrawRect(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth - kButtonSpacing,
                 kBottomScreenHeight - kButtonHeight - kButtonSpacing, kButtonArrowWidth, kButtonHeight,
                 row_offset + kTagRows < tag_rows.size() ? clrButtons : clrButtonsDisabled);

        // Navigation arrows
        DrawLeftArrow((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing + kButtonArrowWidth / 2, kBottomScreenHeight - kButtonHeight / 2, kButtonArrowSize,
                      row_offset > 0 ? clrBlack : clrBackground);
        DrawRightArrow(kBottomScreenWidth - ((kBottomScreenWidth - kMenuWidth) / 2) - kButtonArrowWidth / 2, kBottomScreenHeight - kButtonHeight / 2,
                       kButtonArrowSize, row_offset + kTagRows < tag_rows.size() ? clrBlack : clrBackground);

        DrawRect((kBottomScreenWidth - kMenuWidth) / 2 + kButtonSpacing * 2 + kButtonArrowWidth, kBottomScreenHeight - kButtonHeight - kButtonSpacing,
                 kMenuWidth - kButtonSpacing * 4 - kButtonArrowWidth * 2, kButtonHeight, clrButtons);
        DrawText(kBottomScreenWidth / 2, kBottomScreenHeight - kButtonHeight + kButtonTextOffsetY, 0.5, clrBlack, "Add Tag");
    }

    if (SetTargetScreen(TargetScreen::kTop)) {
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, top_title);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 15, 0.4, clrWhite, "Touch and hold a tag to edit");

        SetTargetScreen(TargetScreen::kTopRight);
        DrawRect(0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 45, 1, clrWhite, top_title);
        DrawText(kTopScreenWidth / 2, kTopScreenHeight - 15, 0.4, clrWhite, "Touch and hold a tag to edit");
    }

    changed = false;
}
}  // namespace ui::tags_menu
