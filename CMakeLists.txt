
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

# init_target (main)

# build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# link_boost ()
# final_target ()

# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")

cmake_minimum_required(VERSION 3.22.1)

project(main)

# Add websocket
include_directories(websocketpp)
# Add jetgpio
include_directories(JETGPIO)
# Add motor-controls
include_directories(motor-controls)

add_library(jetgpio JETGPIO/nano.c JETGPIO/jetgpio.h)
target_compile_features(jetgpio PUBLIC cxx_std_17)

# add client.cpp as the main file
add_executable(main)
target_sources(main PRIVATE src/main.cpp)
target_compile_features(main PRIVATE cxx_std_17)

target_link_libraries(main jetgpio)

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