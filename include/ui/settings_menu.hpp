#ifndef UI_SETTINGS_MENU_HPP_
#define UI_SETTINGS_MENU_HPP_

#include <set>
#include <string>
#include <vector>

#include "ui.hpp"

namespace ui::settings_menu {

void Show(void (*callback)());

void Input();
void Render(bool force);
}  // namespace ui::settings_menu

#endif  // UI_SETTINGS_MENU_HPP_
