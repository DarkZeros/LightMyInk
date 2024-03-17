set(includedirs
    NTPClient
)

set(requires
)

set(privrequires
    arduino
)

set(srcs 
    NTPClient/NTPClient.cpp 
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
