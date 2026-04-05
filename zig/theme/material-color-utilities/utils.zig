const std = @import("std");

pub const Argb = u32;

pub const Vec3 = struct { a: f64 = 0, b: f64 = 0, c: f64 = 0 };

pub fn redFromInt(argb: Argb) i32 {
    return @intCast((argb & 0x00ff0000) >> 16);
}

pub fn greenFromInt(argb: Argb) i32 {
    return @intCast((argb & 0x0000ff00) >> 8);
}

pub fn blueFromInt(argb: Argb) i32 {
    return @intCast(argb & 0x000000ff);
}

pub fn argbFromRgb(r: i32, g: i32, b: i32) Argb {
    return 0xFF000000 |
        (@as(u32, @intCast(r & 0xff)) << 16) |
        (@as(u32, @intCast(g & 0xff)) << 8) |
        @as(u32, @intCast(b & 0xff));
}

pub fn argbFromLinrgb(linrgb: Vec3) Argb {
    return argbFromRgb(delinearized(linrgb.a), delinearized(linrgb.b), delinearized(linrgb.c));
}

pub fn delinearized(rgb_component: f64) i32 {
    const normalized = rgb_component / 100.0;
    const dl: f64 = if (normalized <= 0.0031308)
        normalized * 12.92
    else
        1.055 * std.math.pow(f64, normalized, 1.0 / 2.4) - 0.055;
    const v = std.math.clamp(@as(i32, @intFromFloat(@round(dl * 255.0))), 0, 255);
    return v;
}

pub fn linearized(rgb_component: i32) f64 {
    const normalized = @as(f64, @floatFromInt(rgb_component)) / 255.0;
    if (normalized <= 0.040449936) {
        return normalized / 12.92 * 100.0;
    } else {
        return std.math.pow(f64, (normalized + 0.055) / 1.055, 2.4) * 100.0;
    }
}

pub fn lstarFromArgb(argb: Argb) f64 {
    const y = 0.2126 * linearized(redFromInt(argb)) +
        0.7152 * linearized(greenFromInt(argb)) +
        0.0722 * linearized(blueFromInt(argb));
    return lstarFromY(y);
}

pub fn yFromLstar(lstar: f64) f64 {
    if (lstar > 8.0) {
        const cube_root = (lstar + 16.0) / 116.0;
        return cube_root * cube_root * cube_root * 100.0;
    } else {
        return lstar / (24389.0 / 27.0) * 100.0;
    }
}

pub fn lstarFromY(y: f64) f64 {
    const e = 216.0 / 24389.0;
    const yn = y / 100.0;
    if (yn <= e) {
        return (24389.0 / 27.0) * yn;
    } else {
        return 116.0 * std.math.cbrt(yn) - 16.0;
    }
}

pub fn intFromLstar(lstar: f64) Argb {
    const c = delinearized(yFromLstar(lstar));
    return argbFromRgb(c, c, c);
}

pub fn sanitizeDegreesDouble(degrees: f64) f64 {
    if (degrees < 0.0) {
        return @rem(degrees, 360.0) + 360.0;
    } else if (degrees >= 360.0) {
        return @rem(degrees, 360.0);
    }
    return degrees;
}

pub fn sanitizeDegreesInt(degrees: i32) i32 {
    if (degrees < 0) {
        return @rem(degrees, 360) + 360;
    } else if (degrees >= 360) {
        return @rem(degrees, 360);
    }
    return degrees;
}

pub fn diffDegrees(a: f64, b: f64) f64 {
    return 180.0 - @abs(@abs(a - b) - 180.0);
}

pub fn signum(num: f64) f64 {
    if (num < 0) return -1.0;
    if (num == 0) return 0.0;
    return 1.0;
}

pub fn matrixMultiply(input: Vec3, matrix: [3][3]f64) Vec3 {
    return Vec3{
        .a = input.a * matrix[0][0] + input.b * matrix[0][1] + input.c * matrix[0][2],
        .b = input.a * matrix[1][0] + input.b * matrix[1][1] + input.c * matrix[1][2],
        .c = input.a * matrix[2][0] + input.b * matrix[2][1] + input.c * matrix[2][2],
    };
}
