
#include "ui/tags_menu.hpp"

#include <citro2d.h>

#include "ui/viewer.hpp"

namespace ui::tags_menu {

const u32 clrOverlay = C2D_Color32(0x11, 0x11, 0x11, 0x7F);

bool has_rendered;

void Show() {
    has_rendered = false;
    SetUiFunctions(Input, Render);
}

void Input() {
    if (keysDown() & KEY_SELECT) {
        viewer::Show();
    }
}
void Render(bool force) {
    if (!force && has_rendered) return;

    viewer::Render(true);

    SetTargetScreen(TargetScreen::BOTTOM);
    C2D_DrawRectSolid(0, 0, 0, kBottomScreenWidth, kBottomScreenHeight, clrOverlay);
    if (SetTargetScreen(TargetScreen::TOP)) {
        C2D_DrawRectSolid(0, 0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
        SetTargetScreen(TargetScreen::TOP_RIGHT);
        C2D_DrawRectSolid(0, 0, 0, kTopScreenWidth, kTopScreenHeight, clrOverlay);
    }

    has_rendered = true;
}
}  // namespace ui::tags_menu
