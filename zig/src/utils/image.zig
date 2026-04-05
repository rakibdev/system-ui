const std = @import("std");

const c = @cImport({
    @cInclude("stdio.h");
    @cInclude("cairo/cairo.h");
    @cInclude("webp/decode.h");
    @cInclude("jpeglib.h");
    @cInclude("setjmp.h");
});

const Argb = u32;

fn surfaceToArgbPixels(surface: *c.cairo_surface_t, allocator: std.mem.Allocator) ![]Argb {
    const width: usize = @intCast(c.cairo_image_surface_get_width(surface));
    const height: usize = @intCast(c.cairo_image_surface_get_height(surface));
    const stride: usize = @intCast(c.cairo_image_surface_get_stride(surface));
    const data = c.cairo_image_surface_get_data(surface);

    var pixels = try allocator.alloc(Argb, width * height);
    for (0..height) |y| {
        for (0..width) |x| {
            const pixel = data + y * stride + x * 4;
            const b = pixel[0];
            const g = pixel[1];
            const r = pixel[2];
            const a = pixel[3];
            pixels[y * width + x] = (@as(u32, a) << 24) | (@as(u32, r) << 16) | (@as(u32, g) << 8) | b;
        }
    }
    return pixels;
}

fn createSurfaceFromWebP(path: [*:0]const u8) ?*c.cairo_surface_t {
    const file = std.fs.openFileAbsoluteZ(path, .{}) catch return null;
    defer file.close();
    const data = file.readToEndAlloc(std.heap.c_allocator, 64 * 1024 * 1024) catch return null;
    defer std.heap.c_allocator.free(data);

    var width: c_int = 0;
    var height: c_int = 0;
    const decoded = c.WebPDecodeRGBA(data.ptr, data.len, &width, &height) orelse return null;
    defer c.WebPFree(decoded);

    const surface = c.cairo_image_surface_create(c.CAIRO_FORMAT_RGB24, width, height);
    if (c.cairo_surface_status(surface) != c.CAIRO_STATUS_SUCCESS) return null;

    const surface_data = c.cairo_image_surface_get_data(surface);
    const stride: usize = @intCast(c.cairo_image_surface_get_stride(surface));
    const w: usize = @intCast(width);
    const h: usize = @intCast(height);
    for (0..h) |y| {
        for (0..w) |x| {
            const src = decoded + (y * w + x) * 4;
            const dst = surface_data + y * stride + x * 4;
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = 0xFF;
        }
    }
    c.cairo_surface_mark_dirty(surface);
    return surface;
}

fn createSurfaceFromPng(path: [*:0]const u8) ?*c.cairo_surface_t {
    const surface = c.cairo_image_surface_create_from_png(path);
    if (c.cairo_surface_status(surface) != c.CAIRO_STATUS_SUCCESS) {
        c.cairo_surface_destroy(surface);
        return null;
    }
    return surface;
}

/// Returns the dominant color ARGB from an image file, or null on error.
pub fn colorFromImage(path: []const u8, fallback: u32) u32 {
    const path_z = std.heap.c_allocator.dupeZ(u8, path) catch return fallback;
    defer std.heap.c_allocator.free(path_z);

    const ext_start = std.mem.lastIndexOfScalar(u8, path, '.') orelse return fallback;
    const ext_raw = path[ext_start..];
    var ext_buf: [8]u8 = undefined;
    const ext = std.ascii.lowerString(&ext_buf, ext_raw);

    var surface: ?*c.cairo_surface_t = null;
    if (std.mem.eql(u8, ext, ".webp")) {
        surface = createSurfaceFromWebP(path_z);
    } else if (std.mem.eql(u8, ext, ".jpg") or std.mem.eql(u8, ext, ".jpeg")) {
        surface = createSurfaceFromJpeg(path_z);
    } else if (std.mem.eql(u8, ext, ".png")) {
        surface = createSurfaceFromPng(path_z);
    } else {
        return fallback;
    }

    const surf = surface orelse return fallback;
    defer c.cairo_surface_destroy(surf);

    const pixels = surfaceToArgbPixels(surf, std.heap.c_allocator) catch return fallback;
    defer std.heap.c_allocator.free(pixels);

    return dominantColor(pixels, fallback);
}

fn createSurfaceFromJpeg(path: [*:0]const u8) ?*c.cairo_surface_t {
    const file = c.fopen(path, "rb") orelse return null;
    defer _ = c.fclose(file);

    var cinfo: c.jpeg_decompress_struct = undefined;
    var jerr: c.jpeg_error_mgr = undefined;
    cinfo.err = c.jpeg_std_error(&jerr);
    c.jpeg_create_decompress(&cinfo);
    c.jpeg_stdio_src(&cinfo, file);
    _ = c.jpeg_read_header(&cinfo, 1);
    _ = c.jpeg_start_decompress(&cinfo);

    const width: c_int = @intCast(cinfo.output_width);
    const height: c_int = @intCast(cinfo.output_height);
    const channels: usize = @intCast(cinfo.output_components);

    const surface = c.cairo_image_surface_create(c.CAIRO_FORMAT_RGB24, width, height);
    if (c.cairo_surface_status(surface) != c.CAIRO_STATUS_SUCCESS) {
        _ = c.jpeg_finish_decompress(&cinfo);
        c.jpeg_destroy_decompress(&cinfo);
        return null;
    }

    const surface_data = c.cairo_image_surface_get_data(surface);
    const stride: usize = @intCast(c.cairo_image_surface_get_stride(surface));
    const w: usize = @intCast(width);
    const row = std.heap.c_allocator.alloc(u8, w * channels) catch {
        _ = c.jpeg_finish_decompress(&cinfo);
        c.jpeg_destroy_decompress(&cinfo);
        return null;
    };
    defer std.heap.c_allocator.free(row);

    for (0..@intCast(height)) |y| {
        var row_ptr: [*c]u8 = row.ptr;
        _ = c.jpeg_read_scanlines(&cinfo, &row_ptr, 1);
        for (0..w) |x| {
            const dst = surface_data + y * stride + x * 4;
            if (channels == 3) {
                dst[0] = row[x * 3 + 2];
                dst[1] = row[x * 3 + 1];
                dst[2] = row[x * 3 + 0];
                dst[3] = 0xFF;
            } else if (channels == 1) {
                const gray = row[x];
                dst[0] = gray;
                dst[1] = gray;
                dst[2] = gray;
                dst[3] = 0xFF;
            }
        }
    }

    c.cairo_surface_mark_dirty(surface);
    _ = c.jpeg_finish_decompress(&cinfo);
    c.jpeg_destroy_decompress(&cinfo);
    return surface;
}

/// Simple dominant color: find the most frequent non-transparent, non-gray pixel.
fn dominantColor(pixels: []const Argb, fallback: u32) u32 {
    var map = std.AutoHashMap(u32, u32).init(std.heap.c_allocator);
    defer map.deinit();

    for (pixels) |px| {
        const a = (px >> 24) & 0xff;
        if (a < 128) continue;
        const r = (px >> 16) & 0xff;
        const g = (px >> 8) & 0xff;
        const b = px & 0xff;
        // skip near-gray pixels
        const max_c = @max(r, @max(g, b));
        const min_c = @min(r, @min(g, b));
        if (max_c - min_c < 20) continue;
        // quantize to reduce noise
        const quantized: u32 = ((r >> 3) << 18) | ((g >> 3) << 10) | ((b >> 3) << 2);
        const entry = map.getOrPut(quantized) catch continue;
        if (!entry.found_existing) entry.value_ptr.* = 0;
        entry.value_ptr.* += 1;
    }

    var best_color: u32 = fallback;
    var best_count: u32 = 0;
    var it = map.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.* > best_count) {
            best_count = entry.value_ptr.*;
            const q = entry.key_ptr.*;
            const r: u32 = ((q >> 18) & 0x1f) << 3;
            const g: u32 = ((q >> 10) & 0x1f) << 3;
            const b: u32 = ((q >> 2) & 0x1f) << 3;
            best_color = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    return best_color;
}
