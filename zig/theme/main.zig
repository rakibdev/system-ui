const std = @import("std");
const color = @import("color.zig");
const material = @import("material.zig");
const image = @import("image");

const DynamicPalette = material.DynamicPalette;

fn usage() void {
    std.fs.File.stdout().writeAll(
        \\  --color <hex|image-path>
        \\      --color "#ff5722"
        \\      --color "./image.png|jpg|webp"
        \\
        \\  --template <file>    Template file with variables.
        \\      --template config.css
        \\
        \\  --css                Output CSS template (default: JSON)
        \\  --light              Use light mode (default: dark)
        \\
    ) catch {};
}

fn generateJson(palette: DynamicPalette, w: anytype) !void {
    try w.print("{{\n", .{});
    try w.print("  \"background\": \"{s}\",\n", .{palette.background});
    try w.print("  \"border\": \"{s}\",\n", .{palette.border});
    try w.print("  \"card\": \"{s}\",\n", .{palette.card});
    try w.print("  \"foreground\": \"{s}\",\n", .{palette.foreground});
    try w.print("  \"hover\": \"{s}\",\n", .{palette.hover});
    try w.print("  \"popover\": \"{s}\",\n", .{palette.popover});
    try w.print("  \"primary\": \"{s}\",\n", .{palette.primary});
    try w.print("  \"primaryForeground\": \"{s}\",\n", .{palette.primaryForeground});
    try w.print("  \"secondary\": \"{s}\",\n", .{palette.secondary});
    try w.print("  \"secondaryForeground\": \"{s}\"\n", .{palette.secondaryForeground});
    try w.print("}}\n", .{});
}

fn generateCss(palette: DynamicPalette, w: anytype) !void {
    try w.print(":root {{\n", .{});
    try w.print("  --background: {s};\n", .{palette.background});
    try w.print("  --border: {s};\n", .{palette.border});
    try w.print("  --card: {s};\n", .{palette.card});
    try w.print("  --foreground: {s};\n", .{palette.foreground});
    try w.print("  --hover: {s};\n", .{palette.hover});
    try w.print("  --popover: {s};\n", .{palette.popover});
    try w.print("  --primary: {s};\n", .{palette.primary});
    try w.print("  --primary-foreground: {s};\n", .{palette.primaryForeground});
    try w.print("  --secondary: {s};\n", .{palette.secondary});
    try w.print("  --secondary-foreground: {s};\n", .{palette.secondaryForeground});
    try w.print("}}\n", .{});
}

fn processTemplate(template: []const u8, palette: DynamicPalette, allocator: std.mem.Allocator, buf: *std.ArrayListUnmanaged(u8)) !void {
    const PaletteEntry = struct { key: []const u8, val: []const u8 };
    const entries = [_]PaletteEntry{
        .{ .key = "background", .val = &palette.background },
        .{ .key = "border", .val = &palette.border },
        .{ .key = "card", .val = &palette.card },
        .{ .key = "foreground", .val = &palette.foreground },
        .{ .key = "hover", .val = &palette.hover },
        .{ .key = "popover", .val = &palette.popover },
        .{ .key = "primary", .val = &palette.primary },
        .{ .key = "primaryForeground", .val = &palette.primaryForeground },
        .{ .key = "secondary", .val = &palette.secondary },
        .{ .key = "secondaryForeground", .val = &palette.secondaryForeground },
    };

    var result = try allocator.dupe(u8, template);
    defer allocator.free(result);

    var i: usize = 0;
    while (i < result.len) {
        if (result[i] != '{') {
            i += 1;
            continue;
        }
        const end = std.mem.indexOfScalarPos(u8, result, i, '}') orelse {
            i += 1;
            continue;
        };
        const inner = result[i + 1 .. end];
        const dot = std.mem.indexOfScalar(u8, inner, '.');
        const key = if (dot) |d| inner[0..d] else inner;
        const variant = if (dot) |d| inner[d + 1 ..] else "";

        var replacement: []const u8 = "";
        for (entries) |e| {
            if (std.mem.eql(u8, e.key, key)) {
                replacement = if (std.mem.eql(u8, variant, "hexDigits"))
                    e.val[1..]
                else
                    e.val;
                break;
            }
        }

        const newResult = try std.mem.concat(allocator, u8, &.{ result[0..i], replacement, result[end + 1 ..] });
        allocator.free(result);
        result = newResult;
        i += replacement.len;
    }

    try buf.appendSlice(allocator, result);
}

pub fn main() !void {
    const allocator = std.heap.c_allocator;

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var colorValue: []const u8 = material.defaultColor;
    var templatePath: []const u8 = "";
    var outputCss = false;
    var darkMode = true;

    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "--help")) {
            usage();
            return;
        } else if (std.mem.eql(u8, arg, "--color") and i + 1 < args.len) {
            i += 1;
            colorValue = args[i];
        } else if (std.mem.eql(u8, arg, "--template") and i + 1 < args.len) {
            i += 1;
            templatePath = args[i];
        } else if (std.mem.eql(u8, arg, "--css")) {
            outputCss = true;
        } else if (std.mem.eql(u8, arg, "--light")) {
            darkMode = false;
        }
    }

    var sourceHex: []const u8 = material.defaultColor;
    var sourceHexOwned: ?[]u8 = null;
    defer if (sourceHexOwned) |s| allocator.free(s);

    if (std.fs.cwd().access(colorValue, .{}) catch null != null) {
        const fallbackArgb = color.argbFromHex(material.defaultColor);
        const dominant = image.colorFromImage(colorValue, fallbackArgb);
        const hexBuf = color.hexFromArgb(dominant);
        sourceHexOwned = try allocator.dupe(u8, &hexBuf);
        sourceHex = sourceHexOwned.?;
    } else if (color.validateHex(colorValue)) {
        sourceHex = colorValue;
    }

    const hct = material.hexToHct(sourceHex);
    const palette = material.createDynamicPalette(hct.hue, hct.chroma, darkMode);

    var outBuf: [65536]u8 = undefined;
    var fbs = std.io.fixedBufferStream(&outBuf);

    if (templatePath.len > 0) {
        const file = std.fs.cwd().openFile(templatePath, .{}) catch |err| {
            var eBuf: [256]u8 = undefined;
            const msg = std.fmt.bufPrint(&eBuf, "Unable to find template: {s}: {}\n", .{ templatePath, err }) catch "";
            std.fs.File.stderr().writeAll(msg) catch {};
            std.process.exit(1);
        };
        defer file.close();
        const content = try file.readToEndAlloc(allocator, 1024 * 1024);
        defer allocator.free(content);
        var tmp = std.ArrayListUnmanaged(u8){};
        defer tmp.deinit(allocator);
        try processTemplate(content, palette, allocator, &tmp);
        std.fs.File.stdout().writeAll(tmp.items) catch {};
        return;
    } else if (outputCss) {
        try generateCss(palette, fbs.writer());
    } else {
        try generateJson(palette, fbs.writer());
    }

    std.fs.File.stdout().writeAll(fbs.getWritten()) catch {};
}
