const std = @import("std");

const colorOff = "\x1b[0m";
const blue = "\x1b[1;34m";
const red = "\x1b[1;31m";
const yellow = "\x1b[0;33m";

fn print(comptime fmt: []const u8, args: anytype) void {
    var buf: [2048]u8 = undefined;
    const msg = std.fmt.bufPrint(&buf, fmt, args) catch return;
    std.fs.File.stdout().writeAll(msg) catch {};
}

pub fn info(msg: []const u8) void {
    print("{s}[info]{s} {s}\n", .{ blue, colorOff, msg });
}

pub fn err(msg: []const u8) void {
    print("{s}[error]{s} {s}\n", .{ red, colorOff, msg });
}

pub fn warn(msg: []const u8) void {
    print("{s}[warn]{s} {s}\n", .{ yellow, colorOff, msg });
}
