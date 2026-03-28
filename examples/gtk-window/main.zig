const std = @import("std");
const c = @cImport({
    @cInclude("gtk/gtk.h");
});

var click_count: u32 = 0;

fn onButtonClicked(widget: ?*c.GtkWidget, data: ?*anyopaque) callconv(.c) void {
    _ = data;
    click_count += 1;

    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrintZ(&buffer, "Clicked {d} time(s)", .{click_count}) catch return;

    c.gtk_button_set_label(@ptrCast(widget), text);
}

fn onActivate(app: ?*c.GtkApplication, user_data: ?*anyopaque) callconv(.c) void {
    _ = user_data;

    const window = c.gtk_application_window_new(app);
    c.gtk_window_set_title(@ptrCast(window), "GTK Zig Window");
    c.gtk_window_set_default_size(@ptrCast(window), 400, 200);

    const button = c.gtk_button_new_with_label("Click Me!");
    _ = c.g_signal_connect_data(button, "clicked", @ptrCast(&onButtonClicked), null, null, c.G_CONNECT_DEFAULT);

    c.gtk_container_add(@ptrCast(window), button);
    c.gtk_widget_show_all(window);
}

pub fn main() void {
    const app = c.gtk_application_new("com.example.zig-gtk", c.G_APPLICATION_FLAGS_NONE);
    defer c.g_object_unref(app);

    _ = c.g_signal_connect_data(app, "activate", @ptrCast(&onActivate), null, null, c.G_CONNECT_DEFAULT);

    const argc: c_int = @intCast(std.os.argv.len);
    const argv: [*c][*c]u8 = @ptrCast(std.os.argv.ptr);
    const status = c.g_application_run(@ptrCast(app), argc, argv);

    std.process.exit(@intCast(status));
}
