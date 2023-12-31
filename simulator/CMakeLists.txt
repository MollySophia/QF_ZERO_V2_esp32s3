cmake_minimum_required(VERSION 3.10)
project(lvgl)
set(CMAKE_C_STANDARD 11)#C11
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

# file(GLOB_RECURSE INCLUDES "lv_drivers/*.h" "lv_demos/*.h"  "lvgl/*.h"  "./*.h" )
# file(GLOB_RECURSE SOURCES  "lv_drivers/*.c" "lv_demos/*.c"  "lvgl/*.c" )


file(GLOB_RECURSE INCLUDES "lv_drivers/*.h" "./*.h" "../main/ui/*.h")
file(GLOB_RECURSE SOURCES  "lv_drivers/*.c" "../main/ui/*.cpp")


add_subdirectory(lvgl)
add_subdirectory(../components/ArduinoJson ArduinoJson.out)
add_subdirectory(../components/mooncake mooncake.out)


# file(GLOB_RECURSE ICONS
#     icons/*.c
#     icons/*.cpp
# )



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) 
SET(CMAKE_CXX_FLAGS "-O3")

find_package(SDL2 REQUIRED SDL2)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories("../main/ui")



add_executable(main main/src/main.cpp main/src/mouse_cursor_icon.c ../main/ui/main_ui.cpp ${SOURCES} ${INCLUDES} ${ICONS})
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)




target_link_libraries(main PRIVATE SDL2 lvgl lvgl::examples lvgl::demos mooncake)



add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main)