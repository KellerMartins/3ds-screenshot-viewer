
#include "ui/tags_menu.hpp"

#include <citro2d.h>

#include "ui/viewer.hpp"

namespace ui::tags_menu {

const u32 clrOverlay = C2D_Color32(0x11, 0x11, 0x11, 0x7F);

bool has_rendered;

void open() {
    has_rendered = false;
    set_ui_functions(input, render);
}

void input() {
    if (keysDown() & KEY_SELECT) {
        viewer::open();
    }
}
void render() {
    if (has_rendered) return;

    set_target_screen(TargetScreen::BOTTOM);

    C2D_DrawRectSolid(0, 0, 0, kBottomScreenWidth, kBottomScreenHeight, clrOverlay);

    has_rendered = true;
}
}  // namespace ui::tags_menu
