set(includedirs
    ArduinoNvs/src
)

set(requires
    nvs_flash
)

set(privrequires
    arduino
)

set(srcs 
    ArduinoNvs/src/ArduinoNvs.cpp 
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})

target_compile_options(${COMPONENT_TARGET} PUBLIC
 -DARDUINONVS_STRING=std::string
 -DARDUINONVS_STRING_INCLUDES=<string>
)