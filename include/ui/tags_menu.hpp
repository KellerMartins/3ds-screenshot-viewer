#ifndef UI_TAGS_MENU_HPP_
#define UI_TAGS_MENU_HPP_

#include <set>
#include <string>
#include <vector>

#include "tags.hpp"
#include "ui.hpp"

namespace ui::tags_menu {

void Show(std::string title, std::set<tags::TagId> selected_tags, void (*callback)(bool, std::set<tags::TagId>));

void Input();
void Render(bool force);
}  // namespace ui::tags_menu

#endif  // UI_TAGS_MENU_HPP_
