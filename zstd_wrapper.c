/*
 * Copyright (c) 2017
 * Satish Patel <satish.patel@linaro.org>
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * zstd_wrapper.c
 *
 * Support for ZSTD compression
 */

#include <stdlib.h>
#include <stdio.h>
#define ZSTD_STATIC_LINKING_ONLY // required as used in uncompress function
#include <zstd.h>

#include "squashfs_fs.h"
#include "compressor.h"

#ifdef ZSTD_DEBUG //To debug zstd stream
void print_buf(void *buf, int size)
{
    int i = 0;
    char *b = (char *) buf;
    fprintf(stdout, "==================================== \n");
    for (i =0 ; i < size; i+=8) {
        fprintf(stderr, "%x %x %x %x %x %x %x %x \n", b[i], b[i+1],
                b[i+2], b[i+3], b[i+4], b[i+5], b[i+6], b[i+7]);
    }
    fprintf(stdout, "==================================== \n");
}
#endif //ZSTD_DEBUG

static int zstd_compress( void *strm,
                          void *dest,
                          void *src,
                          int size,
                          int block_size,
		                  int *error)
{
	/* decompression length */
    size_t const dest_size = ZSTD_compressBound(size);
    if (dest_size >= block_size) {
        fprintf(stderr, "zstd: compression overflow, expsize:0x%lx, \
                outsize:0x%x, ignore stream \n", dest_size, block_size);
        return 0;
    }

    size_t const csize = ZSTD_compress(dest, block_size , src, size, 1);
    if (ZSTD_isError(csize)) {
        fprintf(stderr, "zstd: error compressing: %s \n",
                ZSTD_getErrorName(csize));
        return -1;
    }
#ifdef ZSTD_DEBUG //To debug zstd stream
    fprintf(stdout, "SOURCE: \n");
    print_buf(src, size);
    fprintf(stdout, "DEST: \n");
    print_buf(dest, csize);
#endif //ZSTD_DEBUG
    return csize;
}


static int zstd_uncompress( void *dest,
                            void *src,
                            int size,
                            int outsize,
                            int *error)
{
    unsigned long long const rsize = ZSTD_findDecompressedSize(src, size);
    if (rsize == ZSTD_CONTENTSIZE_ERROR) {
        fprintf(stderr, "zstd: stream is not compressed by zstd.\n");
        return -1;
    } else if (rsize == ZSTD_CONTENTSIZE_UNKNOWN) {
        fprintf(stderr,"zstd: not supported, yet to add streaming support \n");
        return -1;
    }
    size_t const dsize = ZSTD_decompress(dest, outsize, src, size);
    if (dsize != rsize) {
        fprintf(stderr, "zstd: error decoding: %s \n", ZSTD_getErrorName(dsize));
        return -1;
    }
    return dsize;
}


struct compressor zstd_comp_ops = {
	.init = NULL,
	.compress = zstd_compress,
	.uncompress = zstd_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = ZSTD_COMPRESSION,
	.name = "zstd",
	.supported = 1
};
