const hctMod = @import("material-color-utilities/hct.zig");
const color = @import("color.zig");

const Hct = hctMod.Hct;

pub const defaultColor = "#00bfff";

pub const DynamicPalette = struct {
    foreground: [7]u8,
    mutedForeground: [7]u8,
    background: [7]u8,
    card: [7]u8,
    popover: [7]u8,
    hover: [7]u8,
    primary: [7]u8,
    primaryForeground: [7]u8,
    secondary: [7]u8,
    secondaryForeground: [7]u8,
    border: [7]u8,
};

fn isBlue(hue: f64) bool {
    return hue >= 200 and hue <= 260;
}

fn getNeutralChroma(hue: f64) f64 {
    return if (isBlue(hue)) 8.0 else 2.0;
}

fn hctToHex(hue: f64, chroma: f64, tone: f64) [7]u8 {
    const h = Hct.fromHct(hue, chroma, tone);
    return color.hexFromArgb(h.toInt());
}

pub fn createDynamicPalette(sourceHue: f64, sourceChroma: f64, dark: bool) DynamicPalette {
    _ = sourceChroma;
    const hue = sourceHue;
    const neutralChroma = getNeutralChroma(hue);
    const primaryChroma = 40.0;
    const secondaryChroma: f64 = if (dark) 26.0 else 32.0;

    return .{
        .foreground = hctToHex(hue, neutralChroma, if (dark) 85 else 15),
        .mutedForeground = hctToHex(hue, neutralChroma, if (dark) 60 else 45),
        .background = hctToHex(hue, neutralChroma, if (dark) 8 else 99),
        .card = hctToHex(hue, neutralChroma * 1.2, if (dark) 12 else 95),
        .popover = hctToHex(hue, neutralChroma * 1.3, if (dark) 13 else 94),
        .hover = hctToHex(hue, neutralChroma * 1.4, if (dark) 15 else 92),
        .primary = hctToHex(hue, primaryChroma, if (dark) 80 else 40),
        .primaryForeground = hctToHex(hue, primaryChroma, if (dark) 20 else 98),
        .secondary = hctToHex(hue, secondaryChroma, if (dark) 30 else 80),
        .secondaryForeground = hctToHex(hue, secondaryChroma, if (dark) 80 else 20),
        .border = hctToHex(hue, neutralChroma, if (dark) 20 else 80),
    };
}

pub fn hexToHct(hex: []const u8) Hct {
    return Hct.fromArgb(color.argbFromHex(hex));
}
