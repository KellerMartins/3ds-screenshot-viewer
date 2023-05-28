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

    ui::Init();
    screenshots::Init();

    ui::Start();
    while (aptMainLoop()) {
        screenshots::Update();

        ui::Input();
        if (ui::PressedExit()) break;

        ui::Render();
    }

    screenshots::Exit();
    ui::Exit();

    return 0;
}
