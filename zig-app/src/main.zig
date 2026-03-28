const std = @import("std");
const log = @import("log.zig");
const config = @import("config.zig");
const daemon = @import("daemon.zig");
const ext = @import("extension.zig");

fn padded(buf: []u8, s: []const u8, width: usize) usize {
    const n = @min(s.len, buf.len);
    @memcpy(buf[0..n], s[0..n]);
    var i = n;
    while (i < width and i < buf.len) : (i += 1) buf[i] = ' ';
    return i;
}

fn usage() void {
    // exact match of C++ Log::table output format
    const blue = "\x1b[1;34m";
    const pink = "\x1b[0;35m";
    const gray = "\x1b[0;90m";
    const off = "\x1b[0m";

    const rows = [_][3][]const u8{
        .{ "daemon",          "",                        "Start background service." },
        .{ "stop",            "daemon",                  "Stop background service." },
        .{ "",                "--css <path> [path...]",  "CSS files." },
        .{ "",                "--watch",                 "Enable file watching for CSS changes." },
        .{ "",                "",                        "" },
        .{ "{filename.so}",   "[...args]",               "Load extension." },
        .{ "stop",            "{filename}",              "Unload extension." },
    };
    const singles = [_][]const u8{
        "e.g.",
        "launcher.so",
        "stop launcher",
        "",
    };

    const stdout = std.fs.File.stdout();

    for (rows) |row| {
        var buf: [512]u8 = undefined;
        var pos: usize = 0;
        pos += (std.fmt.bufPrint(buf[pos..], "  {s}", .{blue}) catch break).len;
        pos += padded(buf[pos..], row[0], 18);
        pos += (std.fmt.bufPrint(buf[pos..], "{s}{s}", .{ off, pink }) catch break).len;
        pos += padded(buf[pos..], row[1], 28);
        pos += (std.fmt.bufPrint(buf[pos..], "{s}{s}{s}\n", .{ off, gray, row[2] }) catch break).len;
        pos += (std.fmt.bufPrint(buf[pos..], "{s}", .{off}) catch break).len;
        stdout.writeAll(buf[0..pos]) catch {};
    }
    for (singles) |s| {
        var buf: [128]u8 = undefined;
        const line = std.fmt.bufPrint(&buf, "  {s}\n", .{s}) catch continue;
        stdout.writeAll(line) catch {};
    }
    var buf: [256]u8 = undefined;
    const line = std.fmt.bufPrint(&buf, "  {s}Daemon Logs:{s}              {s}{s}{s}\n", .{
        blue, off, gray, config.LOG_FILE, off,
    }) catch return;
    stdout.writeAll(line) catch {};
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    try config.init(alloc);

    daemon.manager = ext.Manager.init(alloc);
    defer daemon.manager.deinit();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    if (args.len < 2) {
        usage();
        return;
    }

    var cmdParts = std.ArrayList(u8){};
    defer cmdParts.deinit(alloc);
    for (args[1..], 0..) |arg, i| {
        if (i > 0) try cmdParts.append(alloc, ' ');
        try cmdParts.appendSlice(alloc, arg);
    }
    const command = cmdParts.items;

    if (std.mem.eql(u8, command, "--help") or std.mem.eql(u8, command, "-h")) {
        usage();
        return;
    }

    if (std.mem.eql(u8, command, "daemon") or std.mem.eql(u8, command, "daemon --serve")) {
        daemon.request(alloc, command) catch {
            log.info("Daemon running.");
            const serve = std.mem.indexOf(u8, command, "--serve") != null;
            _ = serve; // daemon.serve handles HTTP based on this flag
            try daemon.serve(alloc);
        };
        return;
    }

    if (std.mem.eql(u8, command, "stop daemon")) {
        daemon.request(alloc, command) catch {
            log.err("Daemon hasn't been started.");
            std.process.exit(1);
        };
        return;
    }

    daemon.request(alloc, command) catch |e| {
        log.err(@errorName(e));
        std.process.exit(1);
    };
}
