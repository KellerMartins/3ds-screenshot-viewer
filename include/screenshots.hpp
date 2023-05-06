#include <3ds.h>

#include <string>

namespace screenshots {
struct Screenshot {
    std::string name;
    std::string path_top;
    std::string path_top_right;
    std::string path_bot;
    u8 *thumbnail;
};

void find();
void load_start();
void load_stop();

const Screenshot get(std::size_t index);
size_t size();
int num_loaded_thumbs();

}  // namespace screenshots
