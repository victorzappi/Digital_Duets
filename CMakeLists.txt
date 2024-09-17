# three arguments can be passed:
# the path of the project to build [there is a default main.cpp in case project path is not provided]
# the name of the bin [default is name of core project]
# debug flag [default is 0]
# like this:
# cmake -DPRJ_PATH=path/to/project -DBIN_NAME=name -DDEBUG=1
# notice that the project path must be relative to D_D's root
# i.e., relative to where this CMakeLists.txt file is

cmake_minimum_required(VERSION 3.5)
project(Digital_Duets)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEBUG)
        set(CMAKE_BUILD_TYPE Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wextra")
else()
        set(CMAKE_BUILD_TYPE Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Wall -Wextra")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


set(BUILD_PROJECT OFF CACHE BOOL "Build Project" FORCE)
add_subdirectory(AudioEngine)
set(ALL_SRC_FILES ${ENGINE_SRC_FILES})
set(ALL_INCLUDE_DIRS ${ENGINE_INCLUDE_DIRS})

list(APPEND ALL_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
include_directories(${ALL_INCLUDE_DIRS})

# get core project sources
file(GLOB_RECURSE CORE_SRC "core/*.cpp")
list(APPEND ALL_SRC_FILES ${CORE_SRC})

# remove trailing slash from project dir, if it exists
string(REGEX REPLACE "/$" "" PRJ_PATH "${PRJ_PATH}")

# if we chose a project...
if(PRJ_PATH)
        # ...we remove the main file of core project
        list(REMOVE_ITEM ALL_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/core/main.cpp")
        # ...we add all sources of project, including its main file 
        file(GLOB_RECURSE PRJ_SRC "${PRJ_PATH}/*.cpp")
        list(APPEND ALL_SRC_FILES ${PRJ_SRC})
        # ...we add project specific includes
        include_directories("${PRJ_PATH}")
        if(NOT BIN_NAME)
                get_filename_component(BIN_NAME "${PRJ_PATH}" NAME) # extract name of project as default bin name
        endif()
else()
        if(NOT BIN_NAME)
                set(BIN_NAME "Digital_Duets") # default bin name
        endif()
endif()


add_executable(${BIN_NAME} ${ALL_SRC_FILES})

target_link_libraries(${BIN_NAME} -pthread -march=native -ffast-math glfw GLEW GLU GL Xrandr Xxf86vm Xi Xinerama X11 Xcursor dl m asound sndfile fftw3f fftw3)

