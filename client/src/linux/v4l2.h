#ifndef _LINUX_V4L2_H
#define _LINUX_V4L2_H

#include <linux/videodev2.h>

typedef struct
{
    int   fd;
    char *dev_name;

    int                width;
    int                height;
    int                format;
    enum v4l2_buf_type buf_type;

    struct buffer *buffers;
    unsigned int   n_buffers;

    char *dump_path;
    FILE *dump_fp;

    int img_print;
    int img_compat;
    int img_print_batch_mode;
    int img_resize;
    int opt_width;
    int opt_height;
} v4l2_handle_t;

void v4l2_init_device(v4l2_handle_t *v4l2_handle);
void v4l2_start_capturing(v4l2_handle_t *v4l2_handle);
void v4l2_cap_loop(v4l2_handle_t *v4l2_handle);

#define V4L2_UVC_SET_CUR 0x01
#define V4L2_UVC_GET_CUR 0x81

#endif
