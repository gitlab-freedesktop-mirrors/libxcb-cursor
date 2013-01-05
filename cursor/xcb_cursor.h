#ifndef XCB_CURSOR_H
#define XCB_CURSOR_H

#include <xcb/xcb.h>

typedef struct xcb_cursor_context_t xcb_cursor_context_t;

int xcb_cursor_context_new(xcb_cursor_context_t **ctx, xcb_connection_t *conn);

xcb_cursor_t xcb_cursor_load_cursor(xcb_cursor_context_t *c, const char *name);

void xcb_cursor_context_free(xcb_cursor_context_t *ctx);

#endif
