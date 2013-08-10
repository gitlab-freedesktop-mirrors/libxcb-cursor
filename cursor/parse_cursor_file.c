/*
 * vim:ts=4:sw=4:expandtab
 *
 * Copyright © 2013 Michael Stapelberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif
#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#include <xcb/xcb.h>

#include "cursor.h"
#include "xcb_cursor.h"

static uint32_t dist(const uint32_t a, const uint32_t b) {
    return (a > b ? (a - b) : (b - a));
}

static uint32_t find_best_size(xcint_cursor_file_t *cf, const uint32_t target, uint32_t *nsizesp) {
    uint32_t best = 0;
    /* Amount of cursors with the best size */
    uint32_t nsizes = 0;
    for (int n = 0; n < cf->header.ntoc; n++) {
        const uint32_t size = cf->tocs[n].subtype;

        if (cf->tocs[n].type != XCURSOR_IMAGE_TYPE)
            continue;

        /* If the distance is less to the target size, this is a better fit. */
        if (best == 0 || dist(size, target) < dist(best, target)) {
            best = size;
            nsizes = 0;
        }

        if (size == best)
            nsizes++;
    }

    *nsizesp = nsizes;
    return best;
}

int parse_cursor_file(xcb_cursor_context_t *c, const int fd, xcint_image_t **images, int *nimg) {
    /* Read the header, verify the magic value. */
    xcint_cursor_file_t cf;
    uint32_t nsizes = 0;
    uint32_t best = 0;
    uint32_t skip = 0;

    read(fd, &(cf.header), sizeof(xcint_file_header_t));
    cf.header.magic = le32toh(cf.header.magic);
    cf.header.header = le32toh(cf.header.header);
    cf.header.version = le32toh(cf.header.version);
    cf.header.ntoc = le32toh(cf.header.ntoc);

    if (cf.header.magic != XCURSOR_MAGIC)
        return -EINVAL;

    if ((skip = (cf.header.header - sizeof(xcint_file_header_t))) > 0)
        if (lseek(fd, skip, SEEK_CUR) == EOF)
            return -EINVAL;

    if (cf.header.ntoc > 0x10000)
        return -EINVAL;

    /* Read the table of contents */
    cf.tocs = malloc(cf.header.ntoc * sizeof(xcint_file_toc_t));
    read(fd, cf.tocs, cf.header.ntoc * sizeof(xcint_file_toc_t));
    for (int n = 0; n < cf.header.ntoc; n++) {
        cf.tocs[n].type = le32toh(cf.tocs[n].type);
        cf.tocs[n].subtype = le32toh(cf.tocs[n].subtype);
        cf.tocs[n].position = le32toh(cf.tocs[n].position);
    }

    /* No images? Invalid file. */
    if ((best = find_best_size(&cf, c->size, &nsizes)) == 0 || nsizes == 0) {
        free(cf.tocs);
        return -EINVAL;
    }

    *nimg = nsizes;
    if ((*images = calloc(nsizes, sizeof(xcint_image_t))) == NULL) {
        free(cf.tocs);
        return -errno;
    }

    for (int n = 0, cnt = 0;
         n < cf.header.ntoc;
         n++) {
        xcint_chunk_header_t chunk;
        /* for convenience */
        xcint_image_t *i = &((*images)[cnt++]);
        uint32_t numpixels = 0;
        uint32_t *p = NULL;

        if (cf.tocs[n].type != XCURSOR_IMAGE_TYPE ||
            cf.tocs[n].subtype != best)
            continue;

        lseek(fd, cf.tocs[n].position, SEEK_SET);
        read(fd, &chunk, sizeof(xcint_chunk_header_t));
        chunk.header = le32toh(chunk.header);
        chunk.type = le32toh(chunk.type);
        chunk.subtype = le32toh(chunk.subtype);
        chunk.version = le32toh(chunk.version);
        /* Sanity check, as libxcursor does it. */
        if (chunk.type != cf.tocs[n].type ||
            chunk.subtype != cf.tocs[n].subtype) {
            free(cf.tocs);
            return -EINVAL;
        }
        read(fd, i, sizeof(xcint_image_t) - sizeof(uint32_t*)); // TODO: better type
        i->width = le32toh(i->width);
        i->height = le32toh(i->height);
        i->xhot = le32toh(i->xhot);
        i->yhot = le32toh(i->yhot);
        i->delay = le32toh(i->delay);

        /* Read the actual image data and convert it to host byte order */
        if (((uint64_t)i->width) * i->height > UINT32_MAX) {
            /* Catch integer overflows */
            free(cf.tocs);
            return -EINVAL;
        }
        numpixels = i->width * i->height;
        i->pixels = malloc(numpixels * sizeof(uint32_t));
        read(fd, i->pixels, numpixels * sizeof(uint32_t));
        p = i->pixels;
        for (int j = 0; j < numpixels; j++, p++)
            *p = le32toh(*p);
    }

    free(cf.tocs);
    return 0;
}
