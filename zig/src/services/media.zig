const std = @import("std");
const c = @cImport(@cInclude("systemd/sd-bus.h"));

const DBUS_DEST = "org.freedesktop.DBus";
const DBUS_PATH = "/org/freedesktop/DBus";
const DBUS_IFACE = "org.freedesktop.DBus";
const PROPS_IFACE = "org.freedesktop.DBus.Properties";
const MPRIS_PATH = "/org/mpris/MediaPlayer2";
const PLAYER_IFACE = "org.mpris.MediaPlayer2.Player";
const MPRIS_PREFIX = "org.mpris.MediaPlayer2.";

pub const Status = enum { Playing, Paused, Stopped };

pub const Player = struct {
    alloc: std.mem.Allocator,
    bus: *c.sd_bus,
    dest: [:0]u8,
    slot: ?*c.sd_bus_slot = null,
    on_change: ?struct { cb: *const fn (*Player, ?*anyopaque) void, ctx: ?*anyopaque } = null,

    status: Status = .Stopped,
    title: []const u8 = "",
    artist: []const u8 = "",
    art_url: []const u8 = "",
    track_id: []const u8 = "",
    duration: u64 = 0,

    pub fn init(alloc: std.mem.Allocator, bus: *c.sd_bus, dest: []const u8) !*Player {
        const self = try alloc.create(Player);
        self.* = .{ .alloc = alloc, .bus = bus, .dest = try alloc.dupeZ(u8, dest) };
        var err = std.mem.zeroes(c.sd_bus_error);
        defer c.sd_bus_error_free(&err);
        var reply: ?*c.sd_bus_message = null;
        defer if (reply) |r| { _ = c.sd_bus_message_unref(r); };
        if (c.sd_bus_call_method(bus, self.dest.ptr, MPRIS_PATH, PROPS_IFACE, "GetAll",
            &err, &reply, "s", PLAYER_IFACE) >= 0) {
            if (reply) |r| self.parseProperties(r);
        }
        return self;
    }

    pub fn deinit(self: *Player) void {
        if (self.slot) |s| _ = c.sd_bus_slot_unref(s);
        self.alloc.free(self.dest);
        self.freeStrings();
        self.alloc.destroy(self);
    }

    pub fn onChange(self: *Player, cb: *const fn (*Player, ?*anyopaque) void, ctx: ?*anyopaque) void {
        self.on_change = .{ .cb = cb, .ctx = ctx };
        var buf: [256]u8 = undefined;
        const match = std.fmt.bufPrintZ(&buf,
            "type='signal',interface='" ++ PROPS_IFACE ++ "',member='PropertiesChanged'" ++
            ",path='" ++ MPRIS_PATH ++ "',sender='{s}'", .{self.dest}) catch return;
        _ = c.sd_bus_add_match(self.bus, &self.slot, match.ptr, onPropertiesChanged, self);
    }

    pub fn playPause(self: *Player) void { self.call("PlayPause"); }
    pub fn next(self: *Player) void { self.call("Next"); }
    pub fn previous(self: *Player) void { self.call("Previous"); }

    pub fn getProgress(self: *Player) u8 {
        var err = std.mem.zeroes(c.sd_bus_error);
        defer c.sd_bus_error_free(&err);
        var reply: ?*c.sd_bus_message = null;
        defer if (reply) |r| { _ = c.sd_bus_message_unref(r); };
        if (c.sd_bus_call_method(self.bus, self.dest.ptr, MPRIS_PATH, PROPS_IFACE, "Get",
            &err, &reply, "ss", PLAYER_IFACE, "Position") < 0) return 0;
        const r = reply orelse return 0;
        var position: i64 = 0;
        if (c.sd_bus_message_enter_container(r, 'v', "x") >= 0) {
            _ = c.sd_bus_message_read_basic(r, 'x', @ptrCast(&position));
            _ = c.sd_bus_message_exit_container(r);
        }
        const pos: u64 = @intCast(@max(0, position));
        return if (self.duration > 0) @intCast(@min(100, pos * 100 / self.duration)) else 0;
    }

    pub fn setProgress(self: *Player, percent: u8) void {
        if (self.track_id.len == 0 or self.duration == 0) return;
        const track_z = self.alloc.dupeZ(u8, self.track_id) catch return;
        defer self.alloc.free(track_z);
        const position: i64 = @intFromFloat(
            @as(f64, @floatFromInt(percent)) / 100.0 * @as(f64, @floatFromInt(self.duration)));
        var err = std.mem.zeroes(c.sd_bus_error);
        defer c.sd_bus_error_free(&err);
        var reply: ?*c.sd_bus_message = null;
        defer if (reply) |r| { _ = c.sd_bus_message_unref(r); };
        _ = c.sd_bus_call_method(self.bus, self.dest.ptr, MPRIS_PATH, PLAYER_IFACE, "SetPosition",
            &err, &reply, "ox", track_z.ptr, position);
    }

    fn call(self: *Player, method: [*:0]const u8) void {
        var err = std.mem.zeroes(c.sd_bus_error);
        defer c.sd_bus_error_free(&err);
        var reply: ?*c.sd_bus_message = null;
        defer if (reply) |r| { _ = c.sd_bus_message_unref(r); };
        _ = c.sd_bus_call_method(self.bus, self.dest.ptr, MPRIS_PATH, PLAYER_IFACE,
            method, &err, &reply, null);
    }

    fn setStr(self: *Player, field: *[]const u8, value: [*:0]const u8) void {
        if (field.len > 0) self.alloc.free(@constCast(field.*));
        const s = std.mem.span(value);
        field.* = if (s.len > 0) self.alloc.dupe(u8, s) catch "" else "";
    }

    fn freeStrings(self: *Player) void {
        if (self.title.len > 0) self.alloc.free(@constCast(self.title));
        if (self.artist.len > 0) self.alloc.free(@constCast(self.artist));
        if (self.art_url.len > 0) self.alloc.free(@constCast(self.art_url));
        if (self.track_id.len > 0) self.alloc.free(@constCast(self.track_id));
    }

    fn parseMetadata(self: *Player, msg: *c.sd_bus_message) void {
        if (c.sd_bus_message_enter_container(msg, 'a', "{sv}") < 0) return;
        defer _ = c.sd_bus_message_exit_container(msg);
        while (c.sd_bus_message_enter_container(msg, 'e', "sv") > 0) {
            defer _ = c.sd_bus_message_exit_container(msg);
            var key: [*:0]const u8 = undefined;
            if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&key)) < 0) continue;
            const k = std.mem.span(key);
            if (std.mem.eql(u8, k, "xesam:title")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "s") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                var str: [*:0]const u8 = undefined;
                if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&str)) >= 0) self.setStr(&self.title, str);
            } else if (std.mem.eql(u8, k, "xesam:artist")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "as") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                if (c.sd_bus_message_enter_container(msg, 'a', "s") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                var str: [*:0]const u8 = undefined;
                if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&str)) >= 0) self.setStr(&self.artist, str);
            } else if (std.mem.eql(u8, k, "mpris:artUrl")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "s") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                var str: [*:0]const u8 = undefined;
                if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&str)) >= 0) {
                    const url = std.mem.span(str);
                    const stripped = if (std.mem.startsWith(u8, url, "file://")) url[7..] else url;
                    if (self.art_url.len > 0) self.alloc.free(@constCast(self.art_url));
                    self.art_url = if (stripped.len > 0) self.alloc.dupe(u8, stripped) catch "" else "";
                }
            } else if (std.mem.eql(u8, k, "mpris:trackid")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "o") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                var str: [*:0]const u8 = undefined;
                if (c.sd_bus_message_read_basic(msg, 'o', @ptrCast(&str)) >= 0) self.setStr(&self.track_id, str);
            } else if (std.mem.eql(u8, k, "mpris:length")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "x") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                _ = c.sd_bus_message_read_basic(msg, 'x', &self.duration);
            } else {
                _ = c.sd_bus_message_skip(msg, "v");
            }
        }
    }

    fn parseProperties(self: *Player, msg: *c.sd_bus_message) void {
        if (c.sd_bus_message_enter_container(msg, 'a', "{sv}") < 0) return;
        defer _ = c.sd_bus_message_exit_container(msg);
        while (c.sd_bus_message_enter_container(msg, 'e', "sv") > 0) {
            defer _ = c.sd_bus_message_exit_container(msg);
            var key: [*:0]const u8 = undefined;
            if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&key)) < 0) continue;
            const k = std.mem.span(key);
            if (std.mem.eql(u8, k, "PlaybackStatus")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "s") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                var str: [*:0]const u8 = undefined;
                if (c.sd_bus_message_read_basic(msg, 's', @ptrCast(&str)) >= 0) {
                    const s = std.mem.span(str);
                    self.status = if (std.mem.eql(u8, s, "Playing")) .Playing
                                  else if (std.mem.eql(u8, s, "Paused")) .Paused
                                  else .Stopped;
                }
            } else if (std.mem.eql(u8, k, "Metadata")) {
                if (c.sd_bus_message_enter_container(msg, 'v', "a{sv}") < 0) continue;
                defer _ = c.sd_bus_message_exit_container(msg);
                self.parseMetadata(msg);
            } else {
                _ = c.sd_bus_message_skip(msg, "v");
            }
        }
    }

    fn onPropertiesChanged(
        msg: ?*c.sd_bus_message,
        data: ?*anyopaque,
        _: ?*c.sd_bus_error,
    ) callconv(.c) c_int {
        const self: *Player = @ptrCast(@alignCast(data));
        if (msg) |m| {
            var iface: [*:0]const u8 = undefined;
            if (c.sd_bus_message_read_basic(m, 's', @ptrCast(&iface)) >= 0 and
                std.mem.eql(u8, std.mem.span(iface), PLAYER_IFACE))
            {
                self.parseProperties(m);
            }
        }
        if (self.on_change) |h| h.cb(self, h.ctx);
        return 0;
    }
};

pub const Media = struct {
    alloc: std.mem.Allocator,
    bus: *c.sd_bus,
    slot: ?*c.sd_bus_slot = null,
    on_players_change: ?struct { cb: *const fn (?*anyopaque) void, ctx: ?*anyopaque } = null,

    pub fn init(alloc: std.mem.Allocator) !Media {
        var bus: ?*c.sd_bus = null;
        if (c.sd_bus_open_user(&bus) < 0 or bus == null) return error.NoDBusSession;
        return .{ .alloc = alloc, .bus = bus.? };
    }

    pub fn deinit(self: *Media) void {
        if (self.slot) |s| _ = c.sd_bus_slot_unref(s);
        _ = c.sd_bus_unref(self.bus);
    }

    pub fn getPlayers(self: *Media) ![]*Player {
        var players = std.ArrayList(*Player){};
        errdefer {
            for (players.items) |p| p.deinit();
            players.deinit(self.alloc);
        }
        var err = std.mem.zeroes(c.sd_bus_error);
        defer c.sd_bus_error_free(&err);
        var reply: ?*c.sd_bus_message = null;
        defer if (reply) |r| { _ = c.sd_bus_message_unref(r); };
        if (c.sd_bus_call_method(self.bus, DBUS_DEST, DBUS_PATH, DBUS_IFACE, "ListNames",
            &err, &reply, null) < 0) return players.toOwnedSlice(self.alloc);
        const r = reply orelse return players.toOwnedSlice(self.alloc);
        if (c.sd_bus_message_enter_container(r, 'a', "s") < 0) return players.toOwnedSlice(self.alloc);
        defer _ = c.sd_bus_message_exit_container(r);
        var name: [*:0]const u8 = undefined;
        while (c.sd_bus_message_read_basic(r, 's', @ptrCast(&name)) > 0) {
            if (std.mem.startsWith(u8, std.mem.span(name), MPRIS_PREFIX)) {
                const player = try Player.init(self.alloc, self.bus, std.mem.span(name));
                try players.append(self.alloc, player);
            }
        }
        return players.toOwnedSlice(self.alloc);
    }

    pub fn onPlayersChange(self: *Media, cb: *const fn (?*anyopaque) void, ctx: ?*anyopaque) void {
        self.on_players_change = .{ .cb = cb, .ctx = ctx };
        const match = "type='signal',sender='" ++ DBUS_DEST ++ "',interface='" ++
            DBUS_IFACE ++ "',member='NameOwnerChanged'";
        _ = c.sd_bus_add_match(self.bus, &self.slot, match, onNameOwnerChanged, self);
    }

    fn onNameOwnerChanged(
        msg: ?*c.sd_bus_message,
        data: ?*anyopaque,
        _: ?*c.sd_bus_error,
    ) callconv(.c) c_int {
        const self: *Media = @ptrCast(@alignCast(data));
        if (msg) |m| {
            var name: [*:0]const u8 = undefined;
            if (c.sd_bus_message_read_basic(m, 's', @ptrCast(&name)) < 0) return 0;
            if (!std.mem.startsWith(u8, std.mem.span(name), MPRIS_PREFIX)) return 0;
        }
        if (self.on_players_change) |h| h.cb(h.ctx);
        return 0;
    }
};
