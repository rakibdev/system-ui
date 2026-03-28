const std = @import("std");
const posix = std.posix;
const log = @import("log.zig");
const config = @import("config.zig");
const ext = @import("extension.zig");
const g = @import("glib_extern.zig");

pub var manager: ext.Manager = undefined;

const maxBuf = 4096;

var channel: ?*g.GIOChannel = null;
var serveHttp = false;

fn destroy(code: u8) noreturn {
    if (channel) |ch| {
        g.g_io_channel_shutdown(ch, 1, null);
        g.g_io_channel_unref(ch);
    }
    std.process.exit(code);
}

fn sendResponse(client: posix.socket_t, content: []const u8, status: u8) void {
    var buf: [maxBuf]u8 = undefined;
    const json = std.fmt.bufPrint(&buf, "{{\"content\":\"{s}\",\"status\":{d}}}", .{ content, status }) catch return;
    _ = posix.send(client, json, 0) catch {};
}

fn onRequest(alloc: std.mem.Allocator, command: []const u8, client: posix.socket_t) void {
    var args = std.ArrayList([]const u8){};
    defer args.deinit(alloc);

    var it = std.mem.tokenizeScalar(u8, command, ' ');
    while (it.next()) |token| args.append(alloc, token) catch return;

    if (args.items.len == 0) {
        sendResponse(client, "Empty command", 127);
        return;
    }

    const cmd = args.items[0];

    if (std.mem.eql(u8, cmd, "daemon")) {
        sendResponse(client, "Daemon already running.", 0);
        return;
    }

    if (std.mem.eql(u8, cmd, "stop") and args.items.len > 1) {
        const name = args.items[1];
        if (std.mem.eql(u8, name, "daemon")) {
            sendResponse(client, "Daemon exited.", 0);
            posix.close(client);
            destroy(0);
        }
        if (manager.find(name)) |found| {
            var keyIt = manager.extensions.iterator();
            while (keyIt.next()) |entry| {
                if (entry.value_ptr.* == found) {
                    const key = alloc.dupe(u8, entry.key_ptr.*) catch return;
                    defer alloc.free(key);
                    manager.unload(key);
                    sendResponse(client, "Extension unloaded", 0);
                    return;
                }
            }
        }
        sendResponse(client, "Extension not running", 1);
        return;
    }

    if (std.mem.endsWith(u8, cmd, ".so")) {
        if (manager.find(cmd) == null) {
            manager.load(cmd) catch |e| {
                sendResponse(client, @errorName(e), 1);
                return;
            };
        }
        if (manager.find(cmd)) |extension| {
            if (args.items.len > 1) {
                const rest = command[cmd.len + 1 ..];
                const restZ = alloc.dupeZ(u8, rest) catch return;
                defer alloc.free(restZ);
                const resp = extension.onRequestFn(extension, restZ);
                sendResponse(client, std.mem.span(resp.content), resp.status);
            } else {
                sendResponse(client, "Extension running", 0);
            }
        } else {
            sendResponse(client, "Extension not found", 1);
        }
        return;
    }

    sendResponse(client, "Unhandled command.", 127);
}

// GIOChannel callback — integrates socket accept into GTK main loop
var serveAlloc: std.mem.Allocator = undefined;

fn onServerEvent(ch: *g.GIOChannel, condition: g.GIOCondition, _: g.gpointer) callconv(.c) g.gboolean {
    if (@intFromEnum(condition) & @intFromEnum(g.G_IO_IN) == 0) return 1;

    const serverFd = g.g_io_channel_unix_get_fd(ch);
    const client = std.posix.accept(@intCast(serverFd), null, null, 0) catch return 1;

    var buf: [maxBuf]u8 = undefined;
    const n = std.posix.recv(client, &buf, 0) catch {
        std.posix.close(client);
        return 1;
    };
    const command = std.mem.trimRight(u8, buf[0..n], " \n\r\t");
    onRequest(serveAlloc, command, client);
    std.posix.close(client);
    return 1;
}

fn startServer() !void {
    posix.unlink(config.SOCKET_FILE) catch {};

    std.fs.makeDirAbsolute(config.TEMP) catch |e| switch (e) {
        error.PathAlreadyExists => {},
        else => return e,
    };

    const server = try posix.socket(posix.AF.UNIX, posix.SOCK.STREAM | posix.SOCK.CLOEXEC, 0);

    var addr = posix.sockaddr.un{ .family = posix.AF.UNIX, .path = undefined };
    @memset(&addr.path, 0);
    @memcpy(addr.path[0..config.SOCKET_FILE.len], config.SOCKET_FILE);

    posix.bind(server, @ptrCast(&addr), @sizeOf(posix.sockaddr.un)) catch {
        log.err("Unable to bind daemon socket.");
        destroy(1);
    };
    posix.listen(server, 3) catch {
        log.err("Unable to listen on daemon socket.");
        destroy(1);
    };

    channel = g.g_io_channel_unix_new(@intCast(server));
    _ = g.g_io_add_watch(channel.?, g.G_IO_IN, onServerEvent, null);
}

fn loadBaseCss(alloc: std.mem.Allocator) void {
    var css = std.ArrayList(u8){};
    defer css.deinit(alloc);

    var buf: [4096]u8 = undefined;
    const configPath = std.fmt.bufPrint(&buf, "{s}/system-ui.json", .{config.configDir}) catch "";
    parseThemeVars(alloc, configPath, &css);
    css.appendSlice(alloc, "\n") catch {};

    const shareDir = std.posix.getenv("SHARE_DIR") orelse "/usr/share/system-ui";
    const basePath = std.fmt.bufPrint(&buf, "{s}/src/default.css", .{shareDir}) catch "";
    appendFile(alloc, basePath, &css);
    appendFile(alloc, config.userCss, &css);

    const cssZ = alloc.dupeZ(u8, css.items) catch return;
    defer alloc.free(cssZ);
    const provider = g.gtk_css_provider_new();
    _ = g.gtk_css_provider_load_from_data(provider, cssZ, -1, null);
    g.gtk_style_context_add_provider_for_screen(
        g.gdk_screen_get_default(),
        @ptrCast(provider),
        g.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION,
    );
}

fn parseThemeVars(alloc: std.mem.Allocator, path: []const u8, css: *std.ArrayList(u8)) void {
    if (path.len == 0) return;
    const f = std.fs.openFileAbsolute(path, .{}) catch return;
    defer f.close();
    const data = f.readToEndAlloc(alloc, 1024 * 1024) catch return;
    defer alloc.free(data);

    const themeStart = std.mem.indexOf(u8, data, "\"theme\"") orelse return;
    const braceStart = std.mem.indexOfPos(u8, data, themeStart, "{") orelse return;
    const braceEnd = std.mem.indexOfPos(u8, data, braceStart + 1, "}") orelse return;
    const inner = data[braceStart + 1 .. braceEnd];

    var it = std.mem.tokenizeScalar(u8, inner, ',');
    while (it.next()) |pair| {
        const colon = std.mem.indexOf(u8, pair, ":") orelse continue;
        const key = std.mem.trim(u8, pair[0..colon], " \t\n\r\"");
        const value = std.mem.trim(u8, pair[colon + 1 ..], " \t\n\r\"");
        if (key.len == 0 or value.len == 0) continue;
        var line: [256]u8 = undefined;
        const l = std.fmt.bufPrint(&line, "@define-color {s} {s};\n", .{ key, value }) catch continue;
        css.appendSlice(alloc, l) catch {};
    }
}

fn appendFile(alloc: std.mem.Allocator, path: []const u8, css: *std.ArrayList(u8)) void {
    if (path.len == 0) return;
    const f = std.fs.openFileAbsolute(path, .{}) catch return;
    defer f.close();
    const content = f.readToEndAlloc(alloc, 1024 * 1024) catch return;
    defer alloc.free(content);
    if (content.len > 0) {
        css.appendSlice(alloc, content) catch {};
        css.appendSlice(alloc, "\n\n") catch {};
    }
}

fn onTerminate(_: c_int) callconv(.c) void {
    destroy(0);
}

pub fn serve(alloc: std.mem.Allocator) !void {
    serveAlloc = alloc;

    try startServer();

    const sigterm = std.posix.SIG.TERM;
    const act = std.posix.Sigaction{
        .handler = .{ .handler = onTerminate },
        .mask = std.mem.zeroes(std.posix.sigset_t),
        .flags = 0,
    };
    std.posix.sigaction(sigterm, &act, null);

    _ = g.g_setenv("GDK_BACKEND", "wayland", 1);
    g.gtk_init(null, null);

    loadBaseCss(alloc);

    log.info("Daemon listening at " ++ config.SOCKET_FILE);

    g.gtk_main();
}

pub fn request(alloc: std.mem.Allocator, command: []const u8) !void {
    _ = alloc;
    const client = try posix.socket(posix.AF.UNIX, posix.SOCK.STREAM, 0);
    defer posix.close(client);

    var addr = posix.sockaddr.un{ .family = posix.AF.UNIX, .path = undefined };
    @memset(&addr.path, 0);
    @memcpy(addr.path[0..config.SOCKET_FILE.len], config.SOCKET_FILE);

    posix.connect(client, @ptrCast(&addr), @sizeOf(posix.sockaddr.un)) catch {
        log.err("Unable to connect daemon. Is it running?");
        return error.DaemonNotRunning;
    };

    _ = try posix.send(client, command, 0);

    var buf: [maxBuf]u8 = undefined;
    const n = try posix.recv(client, &buf, 0);
    if (n == 0) {
        log.err("Daemon did not respond. Crashed?");
        return;
    }

    const body = buf[0..n];
    if (std.mem.indexOf(u8, body, "\"content\":\"")) |start| {
        const from = start + 11;
        if (std.mem.indexOf(u8, body[from..], "\"")) |end| {
            log.info(body[from .. from + end]);
        }
    } else {
        log.info(body);
    }
}

pub fn runInBackground() !void {
    const pid = try posix.fork();
    if (pid > 0) std.process.exit(0);
    _ = posix.setsid();
    const devNull = try posix.open("/dev/null", .{ .ACCMODE = .RDWR }, 0);
    try posix.dup2(devNull, posix.STDIN_FILENO);
    try posix.dup2(devNull, posix.STDOUT_FILENO);
    try posix.dup2(devNull, posix.STDERR_FILENO);
    posix.close(devNull);
}
