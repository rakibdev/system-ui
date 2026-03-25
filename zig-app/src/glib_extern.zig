pub const gboolean = c_int;
pub const gint = c_int;
pub const guint = c_uint;
pub const gpointer = ?*anyopaque;

pub const GIOChannel = opaque {};
pub const GIOCondition = enum(c_uint) {
    in = 1,
    out = 4,
    pri = 2,
    err = 8,
    hup = 16,
    nval = 32,
};
pub const G_IO_IN: GIOCondition = .in;

pub const GIOFunc = *const fn (channel: *GIOChannel, condition: GIOCondition, data: gpointer) callconv(.c) gboolean;

pub extern fn g_io_channel_unix_new(fd: c_int) *GIOChannel;
pub extern fn g_io_channel_unix_get_fd(channel: *GIOChannel) c_int;
pub extern fn g_io_add_watch(channel: *GIOChannel, condition: GIOCondition, func: GIOFunc, data: gpointer) guint;
pub extern fn g_io_channel_shutdown(channel: *GIOChannel, flush: gboolean, err: ?*?*anyopaque) void;
pub extern fn g_io_channel_unref(channel: *GIOChannel) void;

pub extern fn g_setenv(variable: [*:0]const u8, value: [*:0]const u8, overwrite: gboolean) gboolean;

pub extern fn gtk_init(argc: ?*c_int, argv: ?*?*?[*:0]u8) void;
pub extern fn gtk_main() void;
pub extern fn gtk_main_quit() void;

pub const GdkScreen = opaque {};
pub const GtkCssProvider = opaque {};
pub const GtkStyleProvider = opaque {};

pub const GTK_STYLE_PROVIDER_PRIORITY_APPLICATION: guint = 600;

pub extern fn gdk_screen_get_default() *GdkScreen;
pub extern fn gtk_css_provider_new() *GtkCssProvider;
pub extern fn gtk_css_provider_load_from_data(provider: *GtkCssProvider, data: [*:0]const u8, length: c_long, err: ?*?*anyopaque) gboolean;
pub extern fn gtk_style_context_add_provider_for_screen(screen: *GdkScreen, provider: *anyopaque, priority: guint) void;
