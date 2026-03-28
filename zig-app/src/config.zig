const std = @import("std");

pub const TEMP = "/tmp/system-ui";
pub const SOCKET_FILE = TEMP ++ "/daemon.sock";
pub const LOG_FILE = TEMP ++ "/daemon.log";

pub var homeDir: []const u8 = undefined;
pub var configDir: []const u8 = undefined;
pub var configFile: []const u8 = undefined;
pub var userCss: []const u8 = undefined;

pub fn init(_: std.mem.Allocator) !void {
    const alloc = std.heap.page_allocator;
    homeDir = std.posix.getenv("HOME") orelse "/root";
    configDir = try std.fmt.allocPrint(alloc, "{s}/.config/system-ui", .{homeDir});
    configFile = try std.fmt.allocPrint(alloc, "{s}/system-ui.json", .{configDir});
    userCss = try std.fmt.allocPrint(alloc, "{s}/system-ui.css", .{configDir});
}

pub const Config = struct {
    darkMode: bool = true,
    watchFiles: bool = true,
};
