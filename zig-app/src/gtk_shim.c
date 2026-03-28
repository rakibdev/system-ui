#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include <webp/decode.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

guint launcher_event_keyval(gpointer event) {
    return ((GdkEventKey *)event)->keyval;
}

gboolean launcher_event_is_secondary_button(gpointer event) {
    return ((GdkEventButton *)event)->button == GDK_BUTTON_SECONDARY;
}

void launcher_drag_source_set(GtkWidget *w, gint button_mask,
                              const char *target_name, guint info,
                              GdkDragAction action) {
    GtkTargetEntry te = { (gchar*)target_name, GTK_TARGET_SAME_APP, info };
    gtk_drag_source_set(w, button_mask, &te, 1, action);
}

void launcher_drag_dest_set(GtkWidget *w, const char *target_name,
                            guint info, GdkDragAction action) {
    GtkTargetEntry te = { (gchar*)target_name, GTK_TARGET_SAME_APP, info };
    // GTK_DEST_DEFAULT_ALL silently drops without a drag-motion handler calling
    // gdk_drag_status(). Use DROP|HIGHLIGHT and wire launcher_drag_motion manually.
    gtk_drag_dest_set(w, GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_HIGHLIGHT, &te, 1, action);
    gtk_drag_dest_set_track_motion(w, TRUE);
}

gboolean launcher_drag_motion(GtkWidget *w, GdkDragContext *ctx,
                              gint x, gint y, guint time, gpointer data) {
    (void)w; (void)x; (void)y; (void)data;
    gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
    return TRUE;
}

void launcher_drag_data_set_filename(GtkSelectionData *sel, const char *filename, int len) {
    gtk_selection_data_set(sel, gtk_selection_data_get_target(sel), 8,
                           (const guchar*)filename, len);
}

static void destroy_widget(GtkWidget *w, gpointer data) {
    (void)data;
    gtk_widget_destroy(w);
}

void shim_container_clear(GtkContainer *container) {
    gtk_container_foreach(container, destroy_widget, NULL);
}

/* ── Image surface helpers (jpeg/webp use C macros that Zig can't call) ── */

cairo_surface_t* shim_create_jpeg_surface(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int w = cinfo.output_width, h = cinfo.output_height, ch = cinfo.output_components;
    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(f);
        return NULL;
    }

    uint8_t *sd = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    uint8_t *row = malloc(w * ch);

    for (int y = 0; y < h; y++) {
        uint8_t *rp = row;
        jpeg_read_scanlines(&cinfo, &rp, 1);
        for (int x = 0; x < w; x++) {
            uint8_t *dst = sd + y * stride + x * 4;
            if (ch == 3) {
                dst[0] = row[x*3+2]; dst[1] = row[x*3+1]; dst[2] = row[x*3]; dst[3] = 0xFF;
            } else if (ch == 1) {
                dst[0] = dst[1] = dst[2] = row[x]; dst[3] = 0xFF;
            }
        }
    }
    free(row);
    cairo_surface_mark_dirty(surf);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    return surf;
}

cairo_surface_t* shim_create_webp_surface(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);

    int w, h;
    uint8_t *decoded = WebPDecodeRGBA(buf, sz, &w, &h);
    free(buf);
    if (!decoded) return NULL;

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
        WebPFree(decoded);
        return NULL;
    }

    uint8_t *sd = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t *src = decoded + (y * w + x) * 4;
            uint8_t *dst = sd + y * stride + x * 4;
            dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = 0xFF;
        }
    }
    cairo_surface_mark_dirty(surf);
    WebPFree(decoded);
    return surf;
}

cairo_surface_t* shim_create_svg_surface(const char *path, int w, int h) {
    GError *err = NULL;
    RsvgHandle *handle = rsvg_handle_new_from_file(path, &err);
    if (!handle) { if (err) g_error_free(err); return NULL; }

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surf);
    RsvgRectangle vp = { 0, 0, w, h };
    GError *rerr = NULL;
    if (!rsvg_handle_render_document(handle, cr, &vp, &rerr)) {
        if (rerr) g_error_free(rerr);
        cairo_destroy(cr);
        cairo_surface_destroy(surf);
        g_object_unref(handle);
        return NULL;
    }
    cairo_destroy(cr);
    g_object_unref(handle);
    return surf;
}
