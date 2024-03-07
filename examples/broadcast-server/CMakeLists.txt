
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

# init_target (simple-broadcast-server)

# build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# link_boost ()
# final_target ()

# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")

cmake_minimum_required(VERSION 3.22.1)

project(simple-broadcast-server)

# Add websocket
include_directories(../../websocketpp)

# add client.cpp as the main file
add_executable(simple-broadcast-server)
target_sources(simple-broadcast-server PRIVATE simple-broadcast-server.cpp)
target_compile_features(simple-broadcast-server PRIVATE cxx_std_17)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

# run
add_custom_target(run
  COMMENT "Running example server"
  COMMAND $<TARGET_FILE:simple-broadcast-server>
  DEPENDS simple-broadcast-server
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)