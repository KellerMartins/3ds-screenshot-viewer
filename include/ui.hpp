#ifndef UI_HPP_
#define UI_HPP_

#include <3ds.h>
#include <citro2d.h>

namespace ui {
constexpr int kTopScreenWidth = 400;
constexpr int kTopScreenHeight = 240;
constexpr int kTopScreenSize = (kTopScreenWidth * kTopScreenHeight * 3);

constexpr int kBottomScreenWidth = 320;
constexpr int kBottomScreenHeight = 240;
constexpr int kBottomScreenSize = (kBottomScreenWidth * kBottomScreenHeight * 3);

constexpr int kThumbnailDownscale = 4;

constexpr int kThumbnailWidth = (kTopScreenWidth / kThumbnailDownscale);
constexpr int kThumbnailHeight = (kTopScreenHeight / kThumbnailDownscale);
constexpr int kThumbnailSize = (kThumbnailWidth * kThumbnailHeight * 3);

enum TargetScreen {
    TOP,
    TOP_RIGHT,
    BOTTOM,
};

void Init();
void Exit();

void Input();
void Render();
void SetUiFunctions(void (*input_fn)(), void (*render_fn)(bool));

void ClearTargetScreen(TargetScreen screen, u32 clear_color = C2D_Color32(0x40, 0x40, 0x40, 0xFF));
bool SetTargetScreen(TargetScreen screen);

void input();
void render();

bool TouchedInRect(touchPosition touch, int x, int y, int w, int h);

bool PressedExit();
}  // namespace ui

#endif  // UI_HPP_
