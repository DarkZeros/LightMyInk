# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Would be amazing to have LTO globally for the whole ELF
# add_compile_options(
#     -flto
# )
# add_link_options(
#     -flto
# )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(LightMyInk)

# Make arduino compile in LTO mode
target_compile_options(__idf_arduino PUBLIC
    -flto
)