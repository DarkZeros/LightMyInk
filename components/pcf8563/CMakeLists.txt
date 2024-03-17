set(includedirs
    pcf8563/src
)

set(requires
)

set(privrequires
    arduino
)

set(srcs 
    pcf8563/src/Rtc_Pcf8563.cpp
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
