#include <3ds.h>

#define LOADBMP_IMPLEMENTATION
#include <loadbmp.hpp>

#include "screenshots.hpp"
#include "settings.hpp"
#include "tags.hpp"
#include "ui.hpp"

int main(int argc, char **argv) {
    settings::Load();
    tags::Load();

    screenshots::Init();
    ui::Init();

    screenshots::LoadThumbnailsStart();

    while (aptMainLoop()) {
        ui::Input();
        if (ui::PressedExit()) break;

        ui::Render();
    }

    screenshots::LoadThumbnailsStop();
    tags::Save();
    ui::Exit();

    return 0;
}
