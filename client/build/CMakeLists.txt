cmake_minimum_required(VERSION 2.8)
project (udb C CXX)

add_definitions (
    -O2
#	-Wno-all -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-implicit-function-declaration
)


###########################
# start udb
###########################
set(UDB_SOURCES
${PROJECT_SOURCE_DIR}/../src/platform_linux.cpp
${PROJECT_SOURCE_DIR}/../src/linux/v4l2.cpp
${PROJECT_SOURCE_DIR}/../src/linux/libuvc.cpp
${PROJECT_SOURCE_DIR}/../src/linux/print_img.cpp

${PROJECT_SOURCE_DIR}/../src/auth.cpp
${PROJECT_SOURCE_DIR}/../src/raw.cpp
${PROJECT_SOURCE_DIR}/../src/file.cpp

${PROJECT_SOURCE_DIR}/../src/list.cpp
${PROJECT_SOURCE_DIR}/../src/shell.cpp
${PROJECT_SOURCE_DIR}/../src/udb.cpp
${PROJECT_SOURCE_DIR}/../src/utils.cpp
)

add_executable(udb  ${UDB_SOURCES})
target_include_directories (udb PUBLIC ${PROJECT_SOURCE_DIR}/../src/)
###########################
# end udb
###########################


###########################
# start libusb
###########################
find_package(Threads REQUIRED)

set(LIBUSB_HEADER
${PROJECT_SOURCE_DIR}/../src/external/libusb/
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/os/
)

set(LIBUSB_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/core.c
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/descriptor.c
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/io.c
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/strerror.c
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/sync.c
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/hotplug.c

${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/os/threads_posix.c
${PROJECT_SOURCE_DIR}/../src/external/libusb//libusb/os/events_posix.c
${PROJECT_SOURCE_DIR}/../src/external/libusb//libusb/os/linux_usbfs.c
#${PROJECT_SOURCE_DIR}/../src/external/libusb//libusb/os/linux_udev.c
${PROJECT_SOURCE_DIR}/../src/external/libusb//libusb/os/linux_netlink.c
)

add_library(libusb ${LIBUSB_SOURCES})
target_include_directories(libusb PUBLIC ${LIBUSB_HEADER})
target_link_libraries(libusb Threads::Threads)
###########################
# end libusb
###########################


###########################
# start libuvc
###########################
set(LIBUVC_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/ctrl.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/ctrl-gen.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/device.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/diag.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/frame.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/init.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/stream.c
${PROJECT_SOURCE_DIR}/../src/external/libuvc/src/misc.c
)

set(LIBUVC_HEADER
${PROJECT_SOURCE_DIR}/../src/external/libuvc/include/
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/
)

configure_file(${PROJECT_SOURCE_DIR}/../src/external/libuvc/include/libuvc/libuvc_config.h.in
  ${PROJECT_SOURCE_DIR}/../src/external/libuvc/include/libuvc/libuvc_config.h
  @ONLY
)

add_library(libuvc ${LIBUVC_SOURCES})
target_include_directories(libuvc PUBLIC ${LIBUVC_HEADER})
###########################
# end libuvc
###########################

target_link_libraries(udb libuvc libusb)
#target_link_libraries(udb)

#static link
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#strip
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_BUILD_TYPE "RELEASE")

include(cross.cmake)
