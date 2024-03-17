set(includedirs
    Adafruit-GFX-Library
    Adafruit_BusIO
)

set(requires
)

set(privrequires 
    arduino
)

set(srcs 
    Adafruit-GFX-Library/Adafruit_GFX.cpp
    Adafruit-GFX-Library/Adafruit_GrayOLED.cpp
    Adafruit-GFX-Library/Adafruit_SPITFT.cpp
    Adafruit-GFX-Library/glcdfont.c
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
