const std = @import("std");
const log = @import("log.zig");

// Run command and return stdout (caller frees)
pub fn run(alloc: std.mem.Allocator, command: []const u8) []u8 {
    var argv = std.ArrayList([]const u8){};
    defer argv.deinit(alloc);

    var it = std.mem.tokenizeScalar(u8, command, ' ');
    while (it.next()) |tok| argv.append(alloc, tok) catch return alloc.dupe(u8, "") catch "";

    var child = std.process.Child.init(argv.items, alloc);
    child.stdout_behavior = .Pipe;
    child.stderr_behavior = .Ignore;
    child.spawn() catch {
        var buf: [256]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "spawn \"{s}\" failed.", .{command}) catch "spawn failed";
        log.err(msg);
        return alloc.dupe(u8, "") catch "";
    };
    const out = child.stdout.?.readToEndAlloc(alloc, 1024 * 1024) catch "";
    _ = child.wait() catch {};
    return out;
}

// Launch a new detached process (fire and forget)
pub fn runNewProcess(alloc: std.mem.Allocator, command: []const u8) void {
    // Parse args respecting quoted strings
    var args = std.ArrayList([]u8){};
    defer {
        for (args.items) |a| alloc.free(a);
        args.deinit(alloc);
    }

    var current = std.ArrayList(u8){};
    defer current.deinit(alloc);

    var inQuotes = false;
    for (command) |ch| {
        if (ch == ' ' and !inQuotes) {
            if (current.items.len > 0) {
                args.append(alloc, alloc.dupe(u8, current.items) catch return) catch return;
                current.clearRetainingCapacity();
            }
        } else if (ch == '"' or ch == '\'') {
            inQuotes = !inQuotes;
        } else {
            current.append(alloc, ch) catch return;
        }
    }
    if (current.items.len > 0)
        args.append(alloc, alloc.dupe(u8, current.items) catch return) catch return;

    if (args.items.len == 0) return;

    // Build null-terminated args for execvp
    var argsZ = std.ArrayList(?[*:0]u8){};
    defer {
        for (argsZ.items) |a| if (a) |p| alloc.free(std.mem.span(p));
        argsZ.deinit(alloc);
    }
    for (args.items) |a| {
        const z = alloc.dupeZ(u8, a) catch return;
        argsZ.append(alloc, z) catch return;
    }
    argsZ.append(alloc, null) catch return;

    const pid = std.posix.fork() catch {
        var buf: [256]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "posix_spawnp \"{s}\" failed.", .{command}) catch "spawn failed";
        log.err(msg);
        return;
    };

    if (pid == 0) {
        // child — exec
        _ = std.posix.execvpeZ(argsZ.items[0].?, @ptrCast(argsZ.items.ptr), @ptrCast(std.os.environ.ptr)) catch {};
        std.process.exit(1);
    }
    // parent — SIGCHLD = SIG_IGN equivalent: just don't wait
}
