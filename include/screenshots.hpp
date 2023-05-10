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

    ScreenshotInfo(std::string name, std::vector<int>& tags) : name(name), tags(tags), has_thumbnail(false){};
};

void init();
void load_thumbnails_start();
void load_thumbnails_stop();

const screenshots::Screenshot load(std::size_t index);
const ScreenshotInfo get_info(std::size_t index);
size_t size();
int num_loaded_thumbs();

}  // namespace screenshots

#endif
