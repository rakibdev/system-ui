const utils = @import("utils.zig");
const cam16 = @import("cam16.zig");
const solver = @import("solver.zig");

pub const Argb = utils.Argb;
pub const Cam = cam16.Cam;

pub const Hct = struct {
    hue: f64,
    chroma: f64,
    tone: f64,
    argb: Argb,

    pub fn fromArgb(argb: Argb) Hct {
        const cam = cam16.camFromInt(argb);
        return .{
            .hue = cam.hue,
            .chroma = cam.chroma,
            .tone = utils.lstarFromArgb(argb),
            .argb = argb,
        };
    }

    pub fn fromHct(hue: f64, chroma: f64, tone: f64) Hct {
        const argb = solver.solveToInt(hue, chroma, tone);
        const cam = cam16.camFromInt(argb);
        return .{
            .hue = cam.hue,
            .chroma = cam.chroma,
            .tone = utils.lstarFromArgb(argb),
            .argb = argb,
        };
    }

    pub fn toInt(self: Hct) Argb {
        return self.argb;
    }
};

pub const lstarFromArgb = utils.lstarFromArgb;
pub const diffDegrees = utils.diffDegrees;
