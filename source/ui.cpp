#include "ui.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <cstring>
#include <iostream>
#include <loadbmp.hpp>

#include "settings.hpp"
#include "ui/viewer.hpp"

namespace ui {

C3D_RenderTarget *top_target;
C3D_RenderTarget *top_right_target;
C3D_RenderTarget *bottom_target;

void (*input_function)() = nullptr;
void (*render_function)(bool) = nullptr;

bool pressed_exit_button = false;

void Init() {
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    top_right_target = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    if (settings::ShowConsole()) consoleInit(GFX_TOP, NULL);

    viewer::Show();
}

void Exit() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void Input() {
    hidScanInput();
    if (keysHeld() & KEY_START) {
        pressed_exit_button = true;
        return;
    }

    if (input_function != nullptr) input_function();
}

void Render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    if (render_function != nullptr) render_function(false);

    C3D_FrameEnd(0);
}

void SetUiFunctions(void (*input_fn)(), void (*render_fn)(bool)) {
    input_function = input_fn;
    render_function = render_fn;
}

bool SetTargetScreen(TargetScreen screen) {
    switch (screen) {
        case TargetScreen::TOP:
            if (settings::ShowConsole()) return false;

            C2D_SceneBegin(top_target);
            break;
        case TargetScreen::TOP_RIGHT:
            if (settings::ShowConsole()) return false;

            C2D_SceneBegin(top_right_target);
            break;
        case TargetScreen::BOTTOM:
            C2D_SceneBegin(bottom_target);
            break;
    }

    return true;
}

void ClearTargetScreen(TargetScreen screen, u32 clear_color) {
    switch (screen) {
        case TargetScreen::TOP:
            C2D_TargetClear(top_target, clear_color);
            break;
        case TargetScreen::TOP_RIGHT:
            C2D_TargetClear(top_right_target, clear_color);
            break;
        case TargetScreen::BOTTOM:
            C2D_TargetClear(bottom_target, clear_color);
            break;
    }
}

bool pressed_exit() { return pressed_exit_button; }
bool TouchedInRect(touchPosition touch, int x, int y, int w, int h) { return touch.px > x && touch.px < x + w && touch.py > y && touch.py < y + h; }

bool PressedExit() { return pressed_exit_button; }
}  // namespace ui
