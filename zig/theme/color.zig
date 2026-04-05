const std = @import("std");

pub fn cleanHex(hex: []const u8) []const u8 {
    if (hex.len > 0 and hex[0] == '#') return hex[1..];
    return hex;
}

pub fn validateHex(hex: []const u8) bool {
    const digits = cleanHex(hex);
    if (digits.len != 6) return false;
    for (digits) |c| {
        if (!std.ascii.isAlphanumeric(c)) return false;
    }
    return true;
}

pub fn argbFromHex(hex: []const u8) u32 {
    const digits = cleanHex(hex);
    return std.fmt.parseInt(u32, digits, 16) catch 0;
}

pub fn hexFromArgb(argb: u32) [7]u8 {
    var buf: [7]u8 = undefined;
    _ = std.fmt.bufPrint(&buf, "#{x:0>6}", .{argb & 0x00FFFFFF}) catch {};
    return buf;
}
