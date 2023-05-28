#ifndef UI_HPP_
#define UI_HPP_

#include <3ds.h>
#include <citro2d.h>

#include <string>

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

constexpr u32 kContrastThreshold = 150;
constexpr u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
constexpr u32 clrButtons = C2D_Color32(0x7F, 0x7F, 0x7F, 0xFF);
constexpr u32 clrButtonsDisabled = C2D_Color32(0x5F, 0x5F, 0x5F, 0xFF);
constexpr u32 clrBackground = C2D_Color32(0x40, 0x40, 0x40, 0xFF);
constexpr u32 clrOverlay = C2D_Color32(0x11, 0x11, 0x11, 0x7F);

enum TargetScreen {
    kTop,
    kTopRight,
    kBottom,
};

enum TextAlignment { kLeft, kCenter, kRight };

void Init();
void Start();
void Exit();

void Input();
void Render();
void SetUiFunctions(void (*input_fn)(), void (*render_fn)(bool));

bool CanRenderTopScreen();
void ClearTargetScreen(TargetScreen screen, u32 clear_color = C2D_Color32(0x40, 0x40, 0x40, 0xFF));
void SetTargetScreen(TargetScreen screen);

void DrawLine(float x0, float y0, float x1, float y1, float thickness, u32 color);
void DrawRect(float x, float y, float width, float height, u32 color);
void DrawOutlineRect(float x, float y, float width, float height, float thickness, u32 color);
void DrawCircle(float x, float y, float radius, u32 color);
void DrawUpArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color = C2D_Color32(0x00, 0x00, 0x00, 0xFF));
void DrawDownArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color = C2D_Color32(0x00, 0x00, 0x00, 0xFF));
void DrawRightArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color = C2D_Color32(0x00, 0x00, 0x00, 0xFF));
void DrawLeftArrow(unsigned int x, unsigned int y, unsigned int scale, u32 color = C2D_Color32(0x00, 0x00, 0x00, 0xFF));
void DrawText(float x, float y, float size, u32 color, std::string text, TextAlignment alignment = TextAlignment::kCenter);
float GetTextWidth(float size, std::string text);
u32 GetApproximateColorBrightness(u32 color);

bool TouchedInRect(touchPosition touch, int x, int y, int w, int h);

float Get3DSlider();
bool Changed3DSlider();

bool PressedExit();
}  // namespace ui

#endif  // UI_HPP_
