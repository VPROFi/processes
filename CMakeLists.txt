cmake_minimum_required(VERSION 3.0.2)

project(processes)

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# build far2l
add_subdirectory(vendor/far2l)

# build plugin
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/far2l)
set(FAR_SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/far2l)
get_directory_property(INSTALL_DIR DIRECTORY ${FAR_SOURCE_DIR} DEFINITION INSTALL_DIR)

add_subdirectory(src)
