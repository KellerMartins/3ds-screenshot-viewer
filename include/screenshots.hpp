#ifndef SCREENSHOTS_HPP_
#define SCREENSHOTS_HPP_

#include <3ds.h>
#include <citro2d.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "tags.hpp"

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

    std::vector<tags::tag_ptr>& tags;

    bool has_thumbnail;
    C2D_Image thumbnail;

    ScreenshotInfo(std::string name, std::vector<tags::tag_ptr>& tags) : name(name), tags(tags), has_thumbnail(false) {}
    bool has_any_tag(std::set<tags::tag_ptr> tags);
    bool has_all_tag(std::set<tags::tag_ptr> tags);
};

enum ScreenshotOrder {
    kTags = 0,
    kTagsNewer = 1,
    kOlder = 2,
    kNewer = 3,

    kFirst = kTags,
    kLast = kNewer,
};

using screenshot_ptr = std::shared_ptr<const Screenshot>;

void Init();
void Update();
void Exit();

void Load(std::size_t index, void (*callback)(screenshot_ptr));
const ScreenshotInfo GetInfo(std::size_t index);
size_t Count();
size_t NumLoadedThumbnails();

const ScreenshotOrder GetOrder();
void SetOrder(ScreenshotOrder order);
void UpdateOrder();
}  // namespace screenshots

#endif  // SCREENSHOTS_HPP_
