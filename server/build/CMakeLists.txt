cmake_minimum_required(VERSION 2.8)
project (udbd C CXX)

add_definitions (
    -O2
)


###########################
# start udbd
###########################
set(UDBD_SOURCES
${PROJECT_SOURCE_DIR}/../src/main.c
${PROJECT_SOURCE_DIR}/../src/net_debugd.c
${PROJECT_SOURCE_DIR}/../src/rlog.c
${PROJECT_SOURCE_DIR}/../src/udb_debug.c
)

add_executable(udbd  ${UDBD_SOURCES})
target_include_directories (udbd PUBLIC ${PROJECT_SOURCE_DIR}/../src/)
###########################
# end udbd
###########################

target_link_libraries(udbd)

#static link
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#strip
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_BUILD_TYPE "RELEASE")
