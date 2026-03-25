const std = @import("std");
const log = @import("log.zig");

pub const Response = struct {
    content: []const u8 = "",
    status: u8 = 0,
};

pub const CResponse = extern struct {
    content: [*:0]const u8,
    status: u8,
};

// C-ABI vtable struct shared with .so extensions
pub const Extension = extern struct {
    handle: ?*anyopaque,
    onRequestFn: *const fn (ext: *Extension, command: [*:0]const u8) callconv(.{ .x86_64_sysv = .{} }) CResponse,
    deinitFn: *const fn (ext: *Extension) callconv(.{ .x86_64_sysv = .{} }) void,
};

pub const Manager = struct {
    alloc: std.mem.Allocator,
    extensions: std.StringHashMap(*Extension),

    pub fn init(alloc: std.mem.Allocator) Manager {
        return .{ .alloc = alloc, .extensions = std.StringHashMap(*Extension).init(alloc) };
    }

    pub fn deinit(self: *Manager) void {
        var keys = std.ArrayList([]const u8){};
        defer keys.deinit(self.alloc);
        var it = self.extensions.keyIterator();
        while (it.next()) |k| keys.append(self.alloc, k.*) catch {};
        for (keys.items) |k| self.unload(k);
        self.extensions.deinit();
    }

    pub fn find(self: *Manager, name: []const u8) ?*Extension {
        var it = self.extensions.iterator();
        while (it.next()) |entry| {
            if (std.mem.indexOf(u8, entry.key_ptr.*, name) != null)
                return entry.value_ptr.*;
        }
        return null;
    }

    pub fn load(self: *Manager, path: []const u8) !void {
        const resolved = try resolvePath(self.alloc, path);
        defer self.alloc.free(resolved);

        std.fs.accessAbsolute(resolved, .{}) catch return error.NotFound;

        const resolvedZ = try self.alloc.dupeZ(u8, resolved);
        defer self.alloc.free(resolvedZ);

        const handle = std.c.dlopen(resolvedZ, .{ .NOW = true }) orelse {
            const dlErrMsg = std.c.dlerror();
            if (dlErrMsg) |msg| {
                log.err(std.mem.span(msg));
            } else {
                log.err("dlopen failed");
            }
            return error.DlopenFailed;
        };

        const sym = std.c.dlsym(handle, "createExtension") orelse {
            _ = std.c.dlclose(handle);
            log.err("dlsym: createExtension not found");
            return error.SymbolNotFound;
        };

        const create: *const fn () callconv(.{ .x86_64_sysv = .{} }) *Extension = @ptrCast(sym);
        const extension = create();
        extension.handle = handle;

        const key = try self.alloc.dupe(u8, resolved);
        try self.extensions.put(key, extension);
    }

    pub fn unload(self: *Manager, id: []const u8) void {
        const extension = self.extensions.get(id) orelse return;
        const handle = extension.handle;
        extension.deinitFn(extension);
        if (handle) |h| _ = std.c.dlclose(h);
        _ = self.extensions.remove(id);
        self.alloc.free(id);
    }
};

fn resolvePath(alloc: std.mem.Allocator, path: []const u8) ![]u8 {
    if (std.fs.path.isAbsolute(path)) return alloc.dupe(u8, path);
    return std.fs.cwd().realpathAlloc(alloc, path) catch {
        const cwd = try std.fs.cwd().realpathAlloc(alloc, ".");
        defer alloc.free(cwd);
        return std.fs.path.join(alloc, &.{ cwd, path });
    };
}
