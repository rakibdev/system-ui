const std = @import("std");
const log = @import("log.zig");

pub fn StorageManager(comptime T: type) type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        file: []const u8,
        loaded: bool = false,
        content: T = .{},

        pub fn init(alloc: std.mem.Allocator, file: []const u8) Self {
            return .{ .alloc = alloc, .file = file };
        }

        pub fn get(self: *Self) *T {
            if (self.loaded) return &self.content;
            self.loaded = true;

            const f = std.fs.openFileAbsolute(self.file, .{}) catch return &self.content;
            defer f.close();

            const data = f.readToEndAlloc(self.alloc, 1024 * 1024) catch return &self.content;
            defer self.alloc.free(data);

            const parsed = std.json.parseFromSlice(T, self.alloc, data, .{ .ignore_unknown_fields = true }) catch |e| {
                var buf: [256]u8 = undefined;
                const msg = std.fmt.bufPrint(&buf, "StorageManager: Parse failed {s}: {s}", .{ self.file, @errorName(e) }) catch "parse error";
                log.err(msg);
                return &self.content;
            };
            defer parsed.deinit();
            self.content = parsed.value;
            return &self.content;
        }

        pub fn save(self: *Self) void {
            const dir = std.fs.path.dirname(self.file) orelse return;
            std.fs.makeDirAbsolute(dir) catch |e| switch (e) {
                error.PathAlreadyExists => {},
                else => {},
            };

            const f = std.fs.createFileAbsolute(self.file, .{}) catch |e| {
                var buf: [256]u8 = undefined;
                const msg = std.fmt.bufPrint(&buf, "StorageManager: Unable to save {s}: {s}", .{ self.file, @errorName(e) }) catch "save error";
                log.err(msg);
                return;
            };
            defer f.close();

            std.json.stringify(self.content, .{}, f.writer()) catch {};
        }
    };
}
