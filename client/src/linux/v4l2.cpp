#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>

#include <linux/usb/video.h>
#include <linux/uvcvideo.h>

#include "linux/print_img.h"
#include "linux/v4l2.h"

#include "utils.h"

/*********************************/
// VL42 Capture
/*********************************/
#define BUFFER_COUNT   2
#define FMT_NUM_PLANES 1

struct buffer
{
    void              *start;
    size_t             length;
    struct v4l2_buffer v4l2_buf;
};

static void v4l2_init_mmap(v4l2_handle_t *v4l2_handle)
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));

    req.count  = BUFFER_COUNT;
    req.type   = v4l2_handle->buf_type;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            printf(
                "%s does not support "
                "memory mapping\n",
                v4l2_handle->dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            exit(EXIT_FAILURE);  // errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2)
    {
        printf("Insufficient buffer memory on %s\n", v4l2_handle->dev_name);
        exit(EXIT_FAILURE);
    }

    v4l2_handle->buffers =
        (struct buffer *)calloc(req.count, sizeof(*(v4l2_handle->buffers)));

    if (!(v4l2_handle->buffers))
    {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (v4l2_handle->n_buffers = 0; v4l2_handle->n_buffers < req.count;
         ++(v4l2_handle->n_buffers))
    {
        struct v4l2_buffer buf;
        struct v4l2_plane  planes[FMT_NUM_PLANES];
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));

        buf.type   = v4l2_handle->buf_type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = v4l2_handle->n_buffers;

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == v4l2_handle->buf_type)
        {
            buf.m.planes = planes;
            buf.length   = FMT_NUM_PLANES;
        }

        if (-1 == ioctl(v4l2_handle->fd, VIDIOC_QUERYBUF, &buf))
            exit(EXIT_FAILURE);

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == v4l2_handle->buf_type)
        {
            v4l2_handle->buffers[v4l2_handle->n_buffers].length =
                buf.m.planes[0].length;
            v4l2_handle->buffers[v4l2_handle->n_buffers].start =
                mmap(NULL /* start anywhere */, buf.m.planes[0].length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */, v4l2_handle->fd,
                     buf.m.planes[0].m.mem_offset);
        }
        else
        {
            v4l2_handle->buffers[v4l2_handle->n_buffers].length = buf.length;
            v4l2_handle->buffers[v4l2_handle->n_buffers].start  = mmap(
                 NULL /* start anywhere */, buf.length,
                 PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */, v4l2_handle->fd, buf.m.offset);
        }

        if (MAP_FAILED == v4l2_handle->buffers[v4l2_handle->n_buffers].start)
            exit(EXIT_FAILURE);
    }
}

void v4l2_init_device(v4l2_handle_t *v4l2_handle)
{
    struct v4l2_capability cap;
    struct v4l2_format     fmt;

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            printf("%s is no V4L2 device\n", v4l2_handle->dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
        !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        printf("%s is not a video capture device, capabilities: %x\n",
               v4l2_handle->dev_name, cap.capabilities);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("%s does not support streaming i/o\n", v4l2_handle->dev_name);
        exit(EXIT_FAILURE);
    }

    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        v4l2_handle->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        v4l2_handle->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    memset(&fmt, 0, sizeof(fmt));

    fmt.type                = v4l2_handle->buf_type;
    fmt.fmt.pix.width       = v4l2_handle->width;
    fmt.fmt.pix.height      = v4l2_handle->height;
    fmt.fmt.pix.pixelformat = v4l2_handle->format;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_S_FMT, &fmt))
    {
        exit(EXIT_FAILURE);
    }

    v4l2_init_mmap(v4l2_handle);
}

void v4l2_start_capturing(v4l2_handle_t *v4l2_handle)
{
    unsigned int       i;
    enum v4l2_buf_type type;

    for (i = 0; i < v4l2_handle->n_buffers; ++i)
    {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type   = v4l2_handle->buf_type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == v4l2_handle->buf_type)
        {
            struct v4l2_plane planes[FMT_NUM_PLANES];
            buf.m.planes = planes;
            buf.length   = FMT_NUM_PLANES;
        }

        if (-1 == ioctl(v4l2_handle->fd, VIDIOC_QBUF, &buf))
        {
            exit(EXIT_FAILURE);
        }
    }

    type = v4l2_handle->buf_type;

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_STREAMON, &type))
    {
        exit(EXIT_FAILURE);
    }
}

static void v4l2_process_buffer(v4l2_handle_t *v4l2_handle, int index, int size)
{
    struct buffer *buff = &(v4l2_handle->buffers[index]);
    if (v4l2_handle->dump_fp)
    {
        fseek(v4l2_handle->dump_fp, 0L, SEEK_SET);
        fwrite(buff->start, size, 1, v4l2_handle->dump_fp);
        fflush(v4l2_handle->dump_fp);
    }

    if (v4l2_handle->img_print)
    {
        if (v4l2_handle->format == V4L2_PIX_FMT_MJPEG)
        {
            if (v4l2_handle->img_print_batch_mode != 1)
            {
                printf("\033[1;0H");
            }

            print_img((char *)buff->start, size, v4l2_handle->opt_width,
                      v4l2_handle->opt_height, v4l2_handle->img_compat);
            printf("ColorView %d*%d\n", v4l2_handle->width,
                   v4l2_handle->height);
        }
        else
        {
            void *bmp_img = malloc(size * 2);
            int   bmp_size =
                depth2bmp_hist((uint16_t *)buff->start, (uint8_t *)bmp_img,
                               v4l2_handle->width, v4l2_handle->height);

            if (v4l2_handle->img_print_batch_mode != 1)
            {
                printf("\033[1;0H");
            }
            print_img((char *)bmp_img, bmp_size, v4l2_handle->opt_width,
                      v4l2_handle->opt_height, v4l2_handle->img_compat);
            printf("DepthView Orignal: %d*%d\n", v4l2_handle->width,
                   v4l2_handle->height);
            free(bmp_img);
        }
    }
}

static void v4l2_read_frame(v4l2_handle_t *v4l2_handle)
{
    struct v4l2_buffer buf;
    int                bytesused;

    memset(&buf, 0, sizeof(buf));

    buf.type   = v4l2_handle->buf_type;
    buf.memory = V4L2_MEMORY_MMAP;

    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == v4l2_handle->buf_type)
    {
        struct v4l2_plane planes[FMT_NUM_PLANES];
        buf.m.planes = planes;
        buf.length   = FMT_NUM_PLANES;
    }

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_DQBUF, &buf))
    {
        exit(EXIT_FAILURE);
    }

    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == v4l2_handle->buf_type)
    {
        bytesused = buf.m.planes[0].bytesused;
    }
    else
    {
        bytesused = buf.bytesused;
    }

    v4l2_process_buffer(v4l2_handle, buf.index, bytesused);
    // printf("bytesused %d\n", bytesused);

    if (-1 == ioctl(v4l2_handle->fd, VIDIOC_QBUF, &buf))
    {
        exit(EXIT_FAILURE);
    }

    return;
}

void v4l2_cap_loop(v4l2_handle_t *v4l2_handle)
{
    unsigned int  count = 0;
    unsigned long read_start_time, read_end_time;
    while (1)
    {
        read_start_time = get_time();
        v4l2_read_frame(v4l2_handle);
        read_end_time = get_time();
        count++;

        printf("FrameNr: %d\t", count);
        printf("time %lu ms\n", read_end_time - read_start_time);
    }
}
