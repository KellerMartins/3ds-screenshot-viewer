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

void init();
void exit();

void input();
void render();

void set_ui_functions(void (*input_fn)(), void (*render_fn)());
bool set_target_screen(TargetScreen screen);
void clear_target_screen(TargetScreen screen, u32 clear_color = C2D_Color32(0x40, 0x40, 0x40, 0xFF));

bool pressed_exit();
}  // namespace ui

#endif
