set(includedirs
)

set(requires
    arduino
    WiFiManager
    NTPClient
    ArduinoJson
    ArduinoNvs
    pcf8563
    GxEPD2
    Time
)

set(privrequires
)

set(srcs 
    #hello_world_main.c
    main.cpp
    #Watchy_7_SEG.cpp
    #WatchyRTC.cpp
    #Watchy.cpp
    display.cpp
    battery.cpp
    core.cpp
    power.cpp
    touch.cpp
    peripherals.cpp
    time.cpp
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})

target_compile_options(${COMPONENT_TARGET} PUBLIC
)