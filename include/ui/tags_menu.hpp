#ifndef UI_TAGS_MENU_HPP_
#define UI_TAGS_MENU_HPP_

#include <set>
#include <string>
#include <vector>

#include "tags.hpp"
#include "ui.hpp"

namespace ui::tags_menu {

void Show(std::string title, bool allow_create_tag, std::set<tags::tag_ptr> selected_tags, void (*callback)(bool, int, std::set<tags::tag_ptr>));

void Input();
void Render(bool force);
}  // namespace ui::tags_menu

#endif  // UI_TAGS_MENU_HPP_
