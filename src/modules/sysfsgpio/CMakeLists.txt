set(SYSFSGPIO_BIN sysfsgpio)

set(SYSFSGPIO_SRCS
    init.cpp
    SysFsGpioModule.cpp
    SysFSGPIOPin.cpp
    SysFsGpioConfig.cpp
)

add_library(${SYSFSGPIO_BIN} SHARED ${SYSFSGPIO_SRCS})

set_target_properties(${SYSFSGPIO_BIN} PROPERTIES
    COMPILE_FLAGS "${MODULE_COMPILE_FLAGS}"
    )

install(TARGETS ${SYSFSGPIO_BIN} DESTINATION ${LEOSAC_MODULE_INSTALL_DIR})
