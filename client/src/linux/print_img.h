#ifndef _PRINT_IMG_H
#define _PRINT_IMG_H

int depth2bmp_hist(uint16_t *depth_img,
                   uint8_t  *bmp_img,
                   int       width,
                   int       height);

int print_img(char        *img,
              int          size,
              unsigned int opt_width,
              unsigned int opt_height,
              int          compat);
#endif
