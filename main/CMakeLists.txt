set(includedirs
)

set(requires
    arduino
    WiFiManager
    NTPClient
    ArduinoJson
    ArduinoNvs
    GfxLibrary
    Time
)

set(privrequires
)

set(srcs 
    main.cpp
    display.cpp
    battery.cpp
    core.cpp
    power.cpp
    touch.cpp
    peripherals.cpp
    time.cpp

    menu.cpp
    watchface.cpp
    watchface_default.cpp

    deep_sleep.cpp
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})

#set_property(TARGET ${COMPONENT_TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(${COMPONENT_TARGET} PUBLIC
    #-flto
)