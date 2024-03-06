cmake_minimum_required(VERSION 3.22.1)

project(server)

# Add websocket
include_directories(../websocketpp)

# add server.cpp as the main file
add_executable(server)
target_sources(server PRIVATE server.cpp)
target_compile_features(server PRIVATE cxx_std_17)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

# run
add_custom_target(run
  COMMENT "Running server"
  COMMAND $<TARGET_FILE:server>
  DEPENDS server
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)