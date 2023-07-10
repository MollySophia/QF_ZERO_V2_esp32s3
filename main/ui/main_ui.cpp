#if SIMULATOR
#include "lvgl/lvgl.h"
#include "lv_examples/lv_demo.h"
#else
#include "lvgl.h"
#include "lv_demos.h"
#endif

#include "mooncake.h"
#include "main_ui.h"

MOONCAKE::Mooncake mooncake;

void main_ui_create() {
    // lv_demo_music();
    mooncake.setDisplay(240, 240);
    mooncake.init();
    mooncake.installBuiltinApps();
    return;
}

void main_ui_update() {
    mooncake.update();
}