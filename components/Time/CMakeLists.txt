set(includedirs
    Time
)

set(requires
)

set(privrequires 
    arduino
)

set(srcs 
    Time/DateStrings.cpp
    Time/Time.cpp
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})

target_compile_options(${COMPONENT_TARGET} PUBLIC
    -flto
)