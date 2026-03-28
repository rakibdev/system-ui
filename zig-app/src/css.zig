const std = @import("std");
const c = @import("gtk.zig");
const log = @import("log.zig");

const CssFile = struct {
    path: []const u8,
    priority: i32 = 0,
};

var globalProvider: ?*c.GtkCssProvider = null;
var cssFiles: std.ArrayList(CssFile) = undefined;
var initialized = false;

pub fn init(alloc: std.mem.Allocator) void {
    cssFiles = std.ArrayList(CssFile).init(alloc);
    initialized = true;
}

pub fn add(alloc: std.mem.Allocator, path: []const u8, priority: i32) void {
    std.fs.accessAbsolute(path, .{}) catch {
        var buf: [512]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "CSS file not found: {s}", .{path}) catch "css not found";
        log.err(msg);
        return;
    };
    for (cssFiles.items) |f| if (std.mem.eql(u8, f.path, path)) return;

    cssFiles.append(.{ .path = alloc.dupe(u8, path) catch return, .priority = priority }) catch return;
    rebuild(alloc);
}

pub fn rebuild(alloc: std.mem.Allocator) void {
    var sorted = cssFiles.clone() catch return;
    defer sorted.deinit();
    std.sort.insertion(CssFile, sorted.items, {}, struct {
        fn lt(_: void, a: CssFile, b: CssFile) bool { return a.priority < b.priority; }
    }.lt);

    var css = std.ArrayList(u8).init(alloc);
    defer css.deinit();

    for (sorted.items) |f| {
        const file = std.fs.openFileAbsolute(f.path, .{}) catch continue;
        defer file.close();
        const content = file.readToEndAlloc(alloc, 1024 * 1024) catch continue;
        defer alloc.free(content);
        if (content.len == 0) continue;
        css.appendSlice(content) catch {};
        css.appendSlice("\n\n") catch {};
    }

    const cssZ = alloc.dupeZ(u8, css.items) catch return;
    defer alloc.free(cssZ);

    if (globalProvider == null)
        globalProvider = c.gtk_css_provider_new();

    _ = c.gtk_css_provider_load_from_data(globalProvider.?, cssZ, -1, null);
    const screen = c.gdk_screen_get_default();
    c.gtk_style_context_add_provider_for_screen(screen, @ptrCast(globalProvider.?), c.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
