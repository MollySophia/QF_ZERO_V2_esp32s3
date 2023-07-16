# Hal
file(GLOB_RECURSE HAL_SRCS
    hal/*.c
    hal/*.cpp
)
set(HAL_INCS
    hal/
)

# Hardware manager
file(GLOB_RECURSE HM_SRCS
    hardware_manager/*.c
    hardware_manager/*.cpp
)
set(HM_INCS
    hardware_manager/
)

# Ui
file(GLOB_RECURSE UI_SRCS
    ui/*.c
    ui/*.cpp
)
set(UI_INCS
    ui/
)

idf_component_register(SRCS "main.cpp" ${HAL_SRCS} ${UI_SRCS} ${HM_SRCS}
                    INCLUDE_DIRS "." ${HAL_INCS} ${UI_INCS} ${HM_INCS})