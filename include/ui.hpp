namespace ui {
constexpr bool kShowConsole = false;

constexpr int kTopScreenWidth = 400;
constexpr int kTopScreenHeight = 240;
constexpr int kTopScreenSize = (kTopScreenWidth * kTopScreenHeight * 3);

constexpr int kBottomScreenWidth = 320;
constexpr int kBottomScreenHeight = 240;
constexpr int kBottomScreenSize = (kBottomScreenWidth * kBottomScreenHeight * 3);

constexpr int kThumbnailDownscale = 4;

constexpr int kThumbnailSpacing = 4;
constexpr int kThumbnailWidth = (kTopScreenWidth / kThumbnailDownscale);
constexpr int kThumbnailHeight = (kTopScreenHeight / kThumbnailDownscale);
constexpr int kThumbnailSize = (kThumbnailWidth * kThumbnailHeight * 3);

constexpr int kNavbarHeight = 24;
constexpr int kNavbarArrowWidth = 64;
constexpr int kNavbarButtonsSpacing = 1;
constexpr int kNavbarIconMargin = 24;
constexpr int kNavbarIconSpacing = 4;
constexpr int kNavbarIconScale = (kNavbarHeight - kNavbarIconSpacing * 2);
constexpr int kNavbarHideButtonWidth = (kBottomScreenWidth - (kNavbarArrowWidth + kNavbarButtonsSpacing) * 2);

constexpr int kNRows = ((kBottomScreenHeight - kNavbarHeight) / kThumbnailHeight);
constexpr int kNCols = (kBottomScreenWidth / (kThumbnailWidth + kThumbnailSpacing));

constexpr int kHMargin = (kBottomScreenWidth - kNCols * kThumbnailWidth - (kNCols - 1) * kThumbnailSpacing) / 2;
constexpr int kVMargin = ((kBottomScreenHeight - kNavbarHeight) - kNRows * kThumbnailHeight - (kNRows - 1) * kThumbnailSpacing) / 2;

constexpr int kSelectionOutline = 2;

constexpr unsigned int kSelectionDebounceTicks = 20;

void init();
void exit();

void input();
void render();

bool pressed_exit();

}  // namespace ui
