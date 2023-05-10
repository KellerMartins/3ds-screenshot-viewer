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
void (*render_function)() = nullptr;

bool pressed_exit_button = false;

void init() {
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    top_right_target = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    if (settings::get_show_console()) consoleInit(GFX_TOP, NULL);

    viewer::open();
}

void exit() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void input() {
    hidScanInput();
    if (keysHeld() & KEY_START) {
        pressed_exit_button = true;
        return;
    }

    if (input_function != nullptr) input_function();
}

void render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    if (render_function != nullptr) render_function();

    C3D_FrameEnd(0);
}

void set_ui_functions(void (*input_fn)(), void (*render_fn)()) {
    input_function = input_fn;
    render_function = render_fn;
}

bool set_target_screen(TargetScreen screen) {
    switch (screen) {
        case TargetScreen::TOP:
            if (settings::get_show_console()) return false;

            C2D_SceneBegin(top_target);
            break;
        case TargetScreen::TOP_RIGHT:
            if (settings::get_show_console()) return false;

            C2D_SceneBegin(top_right_target);
            break;
        case TargetScreen::BOTTOM:
            C2D_SceneBegin(bottom_target);
            break;
    }

    return true;
}

void clear_target_screen(TargetScreen screen, u32 clear_color) {
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
}  // namespace ui
