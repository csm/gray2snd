/* vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab tw=72:
 *
 * image -- Fetch image data.
 * Copyright (C) 2003  Casey Marshall <rsdio@metastatic.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 *
 *    Free Software Foundation, Inc.,
 *    59 Temple Place, Suite 330,
 *    Boston, MA  02111-1307
 *    USA
 *
 * --------------------------------------------------------------------------
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "image.h"

/*
 * Read an eight-bit grayscale image into memory, and return the pointer
 * to the (w * h)-sized portion of memory where it resides.
 *
 *   const char *fname  The file name to read from.
 *   int w              The width of the image, in pixels.
 *   int h              The height of the image, in pixels.
 *
 *   Returns a pointer to the image data or NULL if an error occurred.
 */
unsigned char *fetch_image(const char *fname, int w, int h)
{
    char *img = NULL;
    int img_fd = open(fname, O_RDONLY);
    ssize_t n;
    extern int verbose;
    
    if (img_fd == -1)
        return NULL;
    img = (unsigned char*) malloc(sizeof(unsigned char) * w * h);
    if (!img)
        return NULL;

    n = read(img_fd, img, w * h);
    close(img_fd);

    if (n < (w * h)) {
        return NULL;
    }

    if (verbose)
        fprintf(stderr, "Read %d by %d bitmap; %d bytes.\n", w, h, n);

    return img;
}
