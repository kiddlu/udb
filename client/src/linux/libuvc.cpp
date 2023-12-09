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

#include "linux/libuvc.h"
#include "linux/print_img.h"

#include "utils.h"

unsigned int frame_count = 0;

void libuvc_cb(uvc_frame_t *frame, void *ptr)
{
    uvc_error_t ret;

    libuvc_handle_t *libuvc_handle = (libuvc_handle_t *)ptr;

    //  printf("callback! frame_format = %d, width = %d, height = %d, length =
    //  %lu, ptr = %p\n",
    //    frame->frame_format, frame->width, frame->height, frame->data_bytes,
    //    ptr);
    unsigned long read_start_time, read_end_time;
    read_start_time = get_time();

    if (libuvc_handle->dump_fp)
    {
        fseek(libuvc_handle->dump_fp, 0L, SEEK_SET);
        fwrite(frame->data, frame->data_bytes, 1, libuvc_handle->dump_fp);
        fflush(libuvc_handle->dump_fp);
    }

    if (libuvc_handle->img_print)
    {
        if (libuvc_handle->format == UVC_FRAME_FORMAT_MJPEG)
        {
            if (libuvc_handle->img_print_batch_mode != 1)
            {
                printf("\033[1;0H");
            }

            print_img((char *)frame->data, frame->data_bytes,
                      libuvc_handle->opt_width, libuvc_handle->opt_height,
                      libuvc_handle->img_compat);
            printf("ColorView %d*%d\n", libuvc_handle->width,
                   libuvc_handle->height);
        }
        else
        {
            void *bmp_img = malloc(frame->data_bytes * 2);
            int   bmp_size =
                depth2bmp_hist((uint16_t *)frame->data, (uint8_t *)bmp_img,
                               libuvc_handle->width, libuvc_handle->height);

            if (libuvc_handle->img_print_batch_mode != 1)
            {
                printf("\033[1;0H");
            }
            print_img((char *)bmp_img, bmp_size, libuvc_handle->opt_width,
                      libuvc_handle->opt_height, libuvc_handle->img_compat);
            printf("DepthView Orignal: %d*%d\n", libuvc_handle->width,
                   libuvc_handle->height);
            free(bmp_img);
        }
    }

    read_end_time = get_time();
    frame_count++;
    printf("FrameNr: %d\t", frame_count);
    printf("time %lu ms\n", read_end_time - read_start_time);
}

void libuvc_init_device(libuvc_handle_t *libuvc_handle)
{
    uvc_error_t res;

    res = uvc_init(&(libuvc_handle->ctx), NULL);

    if (res < 0)
    {
        uvc_perror(res, "uvc_init");
        return;
    }

    res = uvc_find_device(
        libuvc_handle->ctx, &(libuvc_handle->dev), libuvc_handle->vid,
        libuvc_handle->pid,
        libuvc_handle
            ->dev_sn); /* filter devices: vendor_id, product_id, "serial_num" */

    if (res < 0)
    {
        uvc_perror(res, "uvc_find_device"); /* no devices found */
    }

    /* Try to open the device: requires exclusive access */
    res = uvc_open(libuvc_handle->dev, &(libuvc_handle->devh));

    if (res < 0)
    {
        uvc_perror(res, "uvc_open"); /* unable to open device */
    }
}

void libuvc_start_capturing(libuvc_handle_t *libuvc_handle)
{
    uvc_error_t res;
    // berxel code
    // try 3 times
    for (int i = 0; i < 3; i++)
    {
        // berxel end

        res = uvc_get_stream_ctrl_format_size(
            libuvc_handle->devh, &(libuvc_handle->ctrl), libuvc_handle->format,
            libuvc_handle->width, libuvc_handle->height, libuvc_handle->fps);

        // berxel code
        if (res == 0)
            break;
    }
    // berxel end

    if (res < 0)
    {
        uvc_perror(res, "get uvc mode, pls restart"); /* device doesn't provide
                                                         a matching stream */
        exit(-1);
    }
    else
    {
        res = uvc_start_streaming(libuvc_handle->devh, &(libuvc_handle->ctrl),
                                  libuvc_cb, (void *)libuvc_handle, 0);

        if (res < 0)
        {
            uvc_perror(res, "start streaming"); /* unable to start stream */
        }
    }
}

void libuvc_cap_loop(libuvc_handle_t *libuvc_handle)
{
    while (1)
    {
        usleep(10 * 1000);
    }

    uvc_close(libuvc_handle->devh);
    uvc_unref_device(libuvc_handle->dev);
    uvc_exit(libuvc_handle->ctx);
}
