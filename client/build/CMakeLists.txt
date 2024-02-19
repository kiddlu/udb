cmake_minimum_required(VERSION 2.8)
project (udb C CXX)

add_definitions (
    -O2
    -DBUILDING_UDB
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

###########################
# start lsusb
###########################
set(LSUSB_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/usbutils/lsusb.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/lsusb-t.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/desc-defs.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/desc-dump.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/names.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/sysfs.c
${PROJECT_SOURCE_DIR}/../src/external/usbutils/usbmisc.c
)

set(LSUSB_HEADER
${PROJECT_SOURCE_DIR}/../src/external/usbutils/
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/
)

add_library(lsusb ${LSUSB_SOURCES})
target_include_directories(lsusb PUBLIC ${LSUSB_HEADER})
###########################
# end lsusb
###########################

###########################
# start adb
###########################
set(ADB_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/adb/adb.c
${PROJECT_SOURCE_DIR}/../src/external/adb/console.c
${PROJECT_SOURCE_DIR}/../src/external/adb/transport.c
${PROJECT_SOURCE_DIR}/../src/external/adb/transport_local.c
${PROJECT_SOURCE_DIR}/../src/external/adb/transport_usb.c
${PROJECT_SOURCE_DIR}/../src/external/adb/commandline.c
${PROJECT_SOURCE_DIR}/../src/external/adb/fdevent.c
${PROJECT_SOURCE_DIR}/../src/external/adb/adb_client.c
${PROJECT_SOURCE_DIR}/../src/external/adb/sockets.c
${PROJECT_SOURCE_DIR}/../src/external/adb/services.c
${PROJECT_SOURCE_DIR}/../src/external/adb/file_sync_client.c
${PROJECT_SOURCE_DIR}/../src/external/adb/usb_vendors.c
${PROJECT_SOURCE_DIR}/../src/external/adb/usb_linux.c
${PROJECT_SOURCE_DIR}/../src/external/adb/get_my_path_linux.c
${PROJECT_SOURCE_DIR}/../src/external/adb/load_file.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_inaddr_any_server.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_local_client.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_local_server.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_loopback_client.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_loopback_server.c
${PROJECT_SOURCE_DIR}/../src/external/adb/socket_network_client.c
)

set(ADB_HEADER
${PROJECT_SOURCE_DIR}/../src/external/adb/
)

add_library(adb ${ADB_SOURCES})
target_include_directories(adb PUBLIC ${ADB_HEADER})
###########################
# end adb
###########################

###########################
# start dfu
###########################
set(DFU_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/main.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfu_load.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfu_util.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfuse.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfuse_mem.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfu.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/dfu_file.c
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/quirks.c
)

set(DFU_HEADER
${PROJECT_SOURCE_DIR}/../src/external/dfu-util/src/
${PROJECT_SOURCE_DIR}/../src/external/libusb/libusb/
)

add_library(dfu ${DFU_SOURCES})
target_include_directories(dfu PUBLIC ${DFU_HEADER})
###########################
# end dfu
###########################

###########################
# start upnpc
###########################
set(UPNPC_SOURCES
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/miniwget.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/minixml.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/igd_desc_parse.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/minisoap.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/miniupnpc.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/upnpreplyparse.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/upnpcommands.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/upnperrors.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/connecthostport.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/portlistingparse.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/receivedata.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/upnpdev.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/minissdpc.c
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/listdevices.c
)

set(UPNPC_HEADER
${PROJECT_SOURCE_DIR}/../src/external/miniupnp/miniupnpc/
)

add_library(upnpc ${UPNPC_SOURCES})
target_include_directories(upnpc PUBLIC ${UPNPC_HEADER})
###########################
# end upnpc
###########################

target_link_libraries(udb libuvc libusb lsusb adb dfu upnpc)
#target_link_libraries(udb)

#static link
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#strip
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_BUILD_TYPE "RELEASE")

include(cross.cmake)
