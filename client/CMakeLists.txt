cmake_minimum_required(VERSION 3.25.0)

project(client)

# add client.cpp as the main file
add_executable(client)
target_sources(client PRIVATE client.cpp)
target_compile_features(client PRIVATE cxx_std_17)

# === boost ===
# find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

include(FetchContent)

# === websocket++ ===
FetchContent_Declare(websocketpp
GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
  GIT_TAG 0.8.2)
FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
  FetchContent_Populate(websocketpp)
  add_subdirectory(${websocketpp_SOURCE_DIR} ${websocketpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# add interface library with all websocketpp dependencies
add_library(Websockets INTERFACE)
target_include_directories(Websockets INTERFACE ${websocketpp_SOURCE_DIR})
target_link_libraries(Websockets INTERFACE Boost::system Boost::thread Boost::regex)

# run
add_custom_target(run
  COMMENT "Running client"
  COMMAND $<TARGET_FILE:client>
  DEPENDS client
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)