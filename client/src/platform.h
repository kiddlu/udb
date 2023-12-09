#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef _WIN32
#include "platform_windows.h"
#else
#include "platform_linux.h"
#endif

void select_usb_dev_by_index(int index, usb_dev_t *usb_dev);
bool get_udb_ctrl_handle_from_usb(usb_dev_t         *usb_dev,
                                  udb_ctrl_handle_t *udb_handle);

bool get_udb_ctrl_handle_from_net(char *hostip, udb_ctrl_handle_t *udb_handle);

bool udb_ctrl_write(udb_ctrl_handle_t *udb_handle,
                    unsigned char     *buffer,
                    unsigned long      size);
bool udb_ctrl_read(udb_ctrl_handle_t *udb_handle,
                   unsigned char     *buffer,
                   unsigned long      size,
                   unsigned long     *BytesReturned);

int print_connected_list(void);
int convert_sn_to_index(char *sn);
#endif
