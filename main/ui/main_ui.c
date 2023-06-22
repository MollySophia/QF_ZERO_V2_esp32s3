#if SIMULATOR
#include "lvgl/lvgl.h"
#include "lv_examples/lv_demo.h"
#else
#include "lvgl.h"
#include "lv_demos.h"
#endif

#include "main_ui.h"

void main_ui_create() {
    lv_demo_music();
    return;
}