
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

# init_target (echo-client)

# build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# link_boost ()
# final_target ()

# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")

cmake_minimum_required(VERSION 3.22.1)

project(echo-client)

# Add websocket
include_directories(../../websocketpp)

# add client.cpp as the main file
add_executable(echo-client)
target_sources(echo-client PRIVATE echo-client.cpp)
target_compile_features(echo-client PRIVATE cxx_std_17)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)

# run
add_custom_target(run
  COMMENT "Running example server"
  COMMAND $<TARGET_FILE:echo-client>
  DEPENDS echo-client
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)