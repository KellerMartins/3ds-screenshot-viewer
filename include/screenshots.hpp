#ifndef SCREENSHOTS_HPP_
#define SCREENSHOTS_HPP_

#include <3ds.h>
#include <citro2d.h>

#include <string>
#include <vector>

namespace screenshots {
struct Screenshot {
    bool is_3d;
    C2D_Image top;
    C2D_Image top_right;
    C2D_Image bottom;
};

struct ScreenshotInfo {
    std::string name;

    std::string path_top;
    std::string path_top_right;
    std::string path_bottom;

    std::vector<int>& tags;

    bool has_thumbnail;
    C2D_Image thumbnail;

    ScreenshotInfo(std::string name, std::vector<int>& tags) : name(name), tags(tags), has_thumbnail(false) {}
};

void Init();
void LoadThumbnailsStart();
void LoadThumbnailsStop();

const screenshots::Screenshot Load(std::size_t index);
const ScreenshotInfo GetInfo(std::size_t index);
size_t Size();
int NumLoadedThumbnails();

}  // namespace screenshots

#endif  // SCREENSHOTS_HPP_
