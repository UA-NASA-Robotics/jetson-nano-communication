
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

# init_target (echo-server)

# build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# link_boost ()
# final_target ()

# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")

cmake_minimum_required(VERSION 3.22.1)

project(echo-server)

# Add websocket
include_directories(../../websocketpp)

include_directories(ljetgpio)

# add client.cpp as the main file
add_executable(echo-server)
target_sources(echo-server PRIVATE echo-server.cpp)
target_compile_features(echo-server PRIVATE cxx_std_17)

# === boost ===
find_package(Boost REQUIRED COMPONENTS filesystem system thread regex)
include_directories(... ${Boost_INCLUDE_DIRS})
link_directories(... ${Boost_LIBRARY_DIRS})
target_link_libraries(echo-server ${Boost_LIBRARIES})
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

# run
add_custom_target(run
  COMMENT "Running example server"
  COMMAND $<TARGET_FILE:echo-server>
  DEPENDS echo-server
  USES_TERMINAL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)