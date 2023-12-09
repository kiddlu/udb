#ifndef _LIBUVC_H
#define _LIBUVC_H

#include "external/libuvc/include/libuvc/libuvc.h"

typedef struct
{
    unsigned short vid;
    unsigned short pid;
    char          *dev_sn;

    int                   width;
    int                   height;
    int                   fps;
    enum uvc_frame_format format;

    uvc_context_t       *ctx;
    uvc_device_t        *dev;
    uvc_device_handle_t *devh;

    uvc_stream_ctrl_t ctrl;

    char *dump_path;
    FILE *dump_fp;

    int img_print;
    int img_compat;
    int img_print_batch_mode;
    int img_resize;
    int opt_width;
    int opt_height;
} libuvc_handle_t;

void libuvc_init_device(libuvc_handle_t *libuvc_handle);
void libuvc_start_capturing(libuvc_handle_t *libuvc_handle);
void libuvc_cap_loop(libuvc_handle_t *libuvc_handle);

#endif
