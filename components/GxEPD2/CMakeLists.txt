set(includedirs
    GxEPD2/src
    GxEPD2/src/bitmaps
    GxEPD2/src/epd
    GxEPD2/src/epd3c
    GxEPD2/src/it8951
)

set(requires
    GfxLibrary
)

set(privrequires 
    arduino
)

set(srcs 
    GxEPD2/src/GxEPD2_EPD.cpp
    GxEPD2/src/epd/GxEPD2_154_D67.cpp
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
