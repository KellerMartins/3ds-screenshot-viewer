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

C2D_TextBuf dynamicBuf, sizeBuf;

void (*input_function)() = nullptr;
void (*render_function)(bool) = nullptr;

bool pressed_exit_button = false;
float slider_3d = 0;
float last_slider_3d = 0;

void Init() {
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    top_right_target = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    sizeBuf = C2D_TextBufNew(512);
    dynamicBuf = C2D_TextBufNew(1024);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    if (settings::ShowConsole()) consoleInit(GFX_TOP, NULL);
}

void Start() { viewer::Show(); }

void Exit() {
    C2D_TextBufDelete(dynamicBuf);
    C2D_TextBufDelete(sizeBuf);

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

    last_slider_3d = slider_3d;
    slider_3d = 1.0f - osGet3DSliderState();

    if (input_function != nullptr) input_function();
}

void Render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    if (render_function != nullptr) render_function(false);

    C2D_TextBufClear(dynamicBuf);
    C2D_TextBufClear(sizeBuf);
    C3D_FrameEnd(0);
}

void SetUiFunctions(void (*input_fn)(), void (*render_fn)(bool)) {
    input_function = input_fn;
    render_function = render_fn;
}
bool CanRenderTopScreen() { return !settings::ShowConsole(); }

void SetTargetScreen(TargetScreen screen) {
    switch (screen) {
        case TargetScreen::kTop:
            C2D_SceneBegin(top_target);
            break;
        case TargetScreen::kTopRight:
            C2D_SceneBegin(top_right_target);
            break;
        case TargetScreen::kBottom:
            C2D_SceneBegin(bottom_target);
            break;
    }
}

void ClearTargetScreen(TargetScreen screen, u32 clear_color) {
    switch (screen) {
        case TargetScreen::kTop:
            C2D_TargetClear(top_target, clear_color);
            break;
        case TargetScreen::kTopRight:
            C2D_TargetClear(top_right_target, clear_color);
            break;
        case TargetScreen::kBottom:
            C2D_TargetClear(bottom_target, clear_color);
            break;
    }
}

void DrawLine(float x0, float y0, float x1, float y1, float thickness, u32 color) { C2D_DrawLine(x0, y0, color, x1, y1, color, thickness, 0); }

void DrawRect(float x, float y, float width, float height, u32 color) { C2D_DrawRectSolid(x, y, 0, width, height, color); }

void DrawOutlineRect(float x, float y, float width, float height, float thickness, u32 color) {
    float half_t = thickness / 2;
    x += half_t;
    y += half_t;
    width -= thickness;
    height -= thickness;

    DrawLine(x - half_t, y, x + width + half_t, y, thickness, color);
    DrawLine(x, y - half_t, x, y + height + half_t, thickness, color);
    DrawLine(x + width, y - half_t, x + width, y + height + half_t, thickness, color);
    DrawLine(x - half_t, y + height, x + width + half_t, y + height, thickness, color);
}

void DrawCircle(float x, float y, float radius, u32 color) { C2D_DrawCircleSolid(x, y, 0, radius, color); }

void DrawUpArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color) {
    C2D_DrawTriangle(x + scale / 2, y + scale / 4, color, x - scale / 2 - 0.5, y + scale / 4, color, x, y - scale / 4, color, 0);
}

void DrawDownArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color) {
    C2D_DrawTriangle(x + scale / 2, y - scale / 4, color, x - scale / 2 - 0.5, y - scale / 4, color, x, y + scale / 4, color, 0);
}

void DrawLeftArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color) {
    C2D_DrawTriangle(x + scale / 4 - 1, y + scale / 2, color, x + scale / 4 - 1, y - scale / 2, color, x - scale / 2 + 2, y, color, 0);
}

void DrawRightArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color) {
    C2D_DrawTriangle(x - scale / 4 + 1, y + scale / 2, color, x - scale / 4 + 1, y - scale / 2, color, x + scale / 2 - 2, y, color, 0);
}

void DrawText(float x, float y, float size, u32 color, std::string text, TextAlignment alignment) {
    C2D_Text t;
    float width, height;
    C2D_TextParse(&t, dynamicBuf, text.c_str());
    C2D_TextGetDimensions(&t, size, size, &width, &height);

    switch (alignment) {
        case TextAlignment::kCenter:
            x -= width / 2;
            break;
        case TextAlignment::kRight:
            x -= width;
            break;
        default:
            break;
    }
    C2D_DrawText(&t, C2D_WithColor, x, y, 0, size, size, color);
}

float GetTextWidth(float size, std::string text) {
    float width;
    C2D_Text t;
    C2D_TextParse(&t, dynamicBuf, text.c_str());
    C2D_TextGetDimensions(&t, size, size, &width, nullptr);
    return width;
}

u32 GetApproximateColorBrightness(u32 color) {
    u32 b = (color >> 16) & 0x000000FF;
    u32 g = (color >> 8) & 0x000000FF;
    u32 r = color & 0x000000FF;

    u32 brightness = (r + r + r + b + g + g + g + g) >> 3;

    return brightness;
}

float Get3DSlider() { return slider_3d; }
bool Changed3DSlider() { return slider_3d != last_slider_3d; }

bool TouchedInRect(touchPosition touch, int x, int y, int w, int h) { return touch.px > x && touch.px < x + w && touch.py > y && touch.py < y + h; }

bool PressedExit() { return pressed_exit_button; }
}  // namespace ui
