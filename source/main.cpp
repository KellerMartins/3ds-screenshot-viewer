#include <3ds.h>

#define LOADBMP_IMPLEMENTATION
#include <loadbmp.hpp>

#include "screenshots.hpp"
#include "settings.hpp"
#include "tags.hpp"
#include "ui.hpp"

int main(int argc, char **argv) {
    settings::load();
    tags::load();

    ui::init();
    screenshots::init();

    screenshots::load_thumbnails_start();

    while (aptMainLoop()) {
        ui::input();
        if (ui::pressed_exit()) break;

        ui::render();
    }

    screenshots::load_thumbnails_stop();
    ui::exit();

    return 0;
}
