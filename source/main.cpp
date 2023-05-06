#include <3ds.h>

#define LOADBMP_IMPLEMENTATION
#include <loadbmp.hpp>

#include "screenshots.hpp"
#include "ui.hpp"

int main(int argc, char **argv) {
    ui::init();

    screenshots::find();
    screenshots::load_start();

    while (aptMainLoop()) {
        ui::input();
        if (ui::pressed_exit()) break;

        ui::render();
    }

    screenshots::load_stop();
    ui::exit();

    return 0;
}
