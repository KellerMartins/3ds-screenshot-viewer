#ifndef UI_SETTINGS_MENU_HPP_
#define UI_SETTINGS_MENU_HPP_

#include <set>
#include <string>

#include "ui.hpp"

namespace ui::settings_menu {

void Show(std::set<std::string> selected_screenshots, void (*callback)(bool));

void Input();
void Render(bool force);
}  // namespace ui::settings_menu

#endif  // UI_SETTINGS_MENU_HPP_
