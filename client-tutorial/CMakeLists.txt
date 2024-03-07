cmake_minimum_required(VERSION 3.22.1)

project(client)

# Add websocket
include_directories(../websocketpp)

# add client.cpp as the main file
add_executable(client)
target_sources(client PRIVATE client.cpp)
target_compile_features(client PRIVATE cxx_std_17)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

# run
add_custom_target(run
  COMMENT "Running client"
  COMMAND $<TARGET_FILE:client>
  DEPENDS client
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)