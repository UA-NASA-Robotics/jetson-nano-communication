file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)
impor

cmake_minimum_required(VERSION 3.22.1)

project(main)

include_directories(../../enet-1.3.18/include)

add_library(enet ../../enet-1.3.18/win32.c ${ENET_HEADER_FILES})
target_compile_features(enet PUBLIC cxx_std_17)

# add client.cpp as the main file
add_executable(main)
target_sources(main PRIVATE udpclient.cpp)
target_compile_features(main PRIVATE cxx_std_17)

target_link_libraries(main enet)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)
include_directories(... ${Boost_INCLUDE_DIRS})
link_directories(... ${Boost_LIBRARY_DIRS})
target_link_libraries(main ${Boost_LIBRARIES})
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

# run
add_custom_target(run
  COMMENT "Running main"
  COMMAND sudo $<TARGET_FILE:main>
  DEPENDS main
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)