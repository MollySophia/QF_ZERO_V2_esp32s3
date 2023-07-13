#if SIMULATOR
#include "lvgl/lvgl.h"
#include "lv_examples/lv_demo.h"
#else
#include "lvgl.h"
#include "lv_demos.h"
#endif

#include "mooncake.h"
#include "apps/launcher/launcher.h"
#include "main_ui.h"

MOONCAKE::Mooncake mooncake;
MOONCAKE::LAUNCHER::Launcher* launcher = new MOONCAKE::LAUNCHER::Launcher;

void main_ui_create() {
    // lv_demo_music();
    MOONCAKE::FrameworkConfig_t config = mooncake.config();
    config.playBootAnim = false;
    mooncake.config(config);
    mooncake.setDisplay(240, 240);
    mooncake.setLauncher(launcher);
    mooncake.init();
    mooncake.installBuiltinApps();
    return;
}

void main_ui_update() {
    mooncake.update();
}