/*
 * vim:ts=4:sw=4:expandtab
 */
#ifndef CURSOR_H
#define CURSOR_H

#include <xcb/render.h>

enum {
    RM_XCURSOR_THEME = 0,
    RM_XCURSOR_SIZE,
    RM_XFT_DPI,
    RM_MAX,
};

typedef struct xcb_cursor_context_t {
    xcb_connection_t *conn;
    xcb_window_t root;

    xcb_font_t cursor_font;

    xcb_render_query_pict_formats_reply_t *pf_reply;

    xcb_render_pictforminfo_t *pict_format;

    /* Specific values of the root window’s RESOURCE_MANAGER atom contents. */
    char *rm[RM_MAX];

    /* Best cursor size. If a file contains multiple cursor images, the images
     * which match the size best will be loaded. */
    uint32_t size;

    const char *home;
    const char *path;
} xcb_cursor_context_t;

/*
 * Cursor files start with a header.  The header
 * contains a magic number, a version number and a
 * table of contents which has type and offset information
 * for the remaining tables in the file.
 *  
 * File minor versions increment for compatible changes
 * File major versions increment for incompatible changes (never, we hope)
 *      
 * Chunks of the same type are always upward compatible.  Incompatible
 * changes are made with new chunk types; the old data can remain under
 * the old type.  Upward compatible changes can add header data as the
 * header lengths are specified in the file.
 *      
 *  File:   
 *      FileHeader
 *      LISTofChunk
 *      
 *  FileHeader:
 *      CARD32          magic       magic number
 *      CARD32          header      bytes in file header
 *      CARD32          version     file version
 *      CARD32          ntoc        number of toc entries
 *      LISTofFileToc   toc         table of contents
 *      
 *  FileToc:
 *      CARD32          type        entry type
 *      CARD32          subtype     entry subtype (size for images)
 *      CARD32          position    absolute file position
 */

/* little-endian */
#define XCURSOR_MAGIC 0x72756358

typedef struct xcint_file_header_t {
    uint32_t magic;
    uint32_t header;
    uint32_t version;
    uint32_t ntoc;
} __attribute__((packed)) xcint_file_header_t;

typedef struct xcint_file_toc_t {
    uint32_t type;
    uint32_t subtype;
    uint32_t position;
} __attribute__((packed)) xcint_file_toc_t;

typedef struct xcint_cursor_file_t {
    xcint_file_header_t header;
    xcint_file_toc_t *tocs;
} xcint_cursor_file_t;

/*
 * The rest of the file is a list of chunks, each tagged by type
 * and version.
 *
 *  Chunk:
 *      ChunkHeader
 *      <extra type-specific header fields>
 *      <type-specific data>
 *
 *  ChunkHeader:
 *      CARD32      header      bytes in chunk header + type header
 *      CARD32      type        chunk type
 *      CARD32      subtype     chunk subtype
 *      CARD32      version     chunk type version
 */

typedef struct xcint_chunk_header_t {
    uint32_t header;
    uint32_t type;
    uint32_t subtype;
    uint32_t version;
} __attribute__((packed)) xcint_chunk_header_t;

#define XCURSOR_IMAGE_TYPE          0xfffd0002
#define XCURSOR_IMAGE_VERSION       1
#define XCURSOR_IMAGE_MAX_SIZE      0x7fff      /* 32767x32767 max cursor size */

typedef struct xcint_image_t {
    uint32_t width;
    uint32_t height;
    uint32_t xhot;
    uint32_t yhot;
    uint32_t delay;
    uint32_t *pixels;
} __attribute__((packed)) xcint_image_t;

/* shape_to_id.c */
const int cursor_shape_to_id(const char *name);

/* parse_cursor_file.c */
int parse_cursor_file(xcb_cursor_context_t *c, const int fd, xcint_image_t **images, int *nimg);

#endif
