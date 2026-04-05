const std = @import("std");
const utils = @import("utils.zig");
const viewing_conditions = @import("viewing_conditions.zig");

const Argb = utils.Argb;
const Vec3 = utils.Vec3;
const ViewingConditions = viewing_conditions.ViewingConditions;
const kDefaultViewingConditions = viewing_conditions.kDefaultViewingConditions;

pub const Cam = struct {
    hue: f64 = 0,
    chroma: f64 = 0,
    j: f64 = 0,
    q: f64 = 0,
    m: f64 = 0,
    s: f64 = 0,
    jstar: f64 = 0,
    astar: f64 = 0,
    bstar: f64 = 0,
};

pub fn camFromIntAndViewingConditions(argb: Argb, vc: ViewingConditions) Cam {
    const red_l = utils.linearized(utils.redFromInt(argb));
    const green_l = utils.linearized(utils.greenFromInt(argb));
    const blue_l = utils.linearized(utils.blueFromInt(argb));
    const x = 0.41233895 * red_l + 0.35762064 * green_l + 0.18051042 * blue_l;
    const y = 0.2126 * red_l + 0.7152 * green_l + 0.0722 * blue_l;
    const z = 0.01932141 * red_l + 0.11916382 * green_l + 0.95034478 * blue_l;

    const r_c = 0.401288 * x + 0.650173 * y - 0.051461 * z;
    const g_c = -0.250268 * x + 1.204414 * y + 0.045854 * z;
    const b_c = -0.002079 * x + 0.048952 * y + 0.953127 * z;

    const r_d = vc.rgb_d[0] * r_c;
    const g_d = vc.rgb_d[1] * g_c;
    const b_d = vc.rgb_d[2] * b_c;

    const r_af = std.math.pow(f64, vc.fl * @abs(r_d) / 100.0, 0.42);
    const g_af = std.math.pow(f64, vc.fl * @abs(g_d) / 100.0, 0.42);
    const b_af = std.math.pow(f64, vc.fl * @abs(b_d) / 100.0, 0.42);
    const r_a = utils.signum(r_d) * 400.0 * r_af / (r_af + 27.13);
    const g_a = utils.signum(g_d) * 400.0 * g_af / (g_af + 27.13);
    const b_a = utils.signum(b_d) * 400.0 * b_af / (b_af + 27.13);

    const a = (11.0 * r_a - 12.0 * g_a + b_a) / 11.0;
    const b = (r_a + g_a - 2.0 * b_a) / 9.0;
    const u = (20.0 * r_a + 20.0 * g_a + 21.0 * b_a) / 20.0;
    const p2 = (40.0 * r_a + 20.0 * g_a + b_a) / 20.0;

    const radians = std.math.atan2(b, a);
    const degrees = radians * 180.0 / std.math.pi;
    const hue = utils.sanitizeDegreesDouble(degrees);
    const hue_radians = hue * std.math.pi / 180.0;
    const ac = p2 * vc.nbb;

    const j = 100.0 * std.math.pow(f64, ac / vc.aw, vc.c * vc.z);
    const q = (4.0 / vc.c) * @sqrt(j / 100.0) * (vc.aw + 4.0) * vc.fl_root;
    const hue_prime = if (hue < 20.14) hue + 360.0 else hue;
    const e_hue = 0.25 * (@cos(hue_prime * std.math.pi / 180.0 + 2.0) + 3.8);
    const p1 = 50000.0 / 13.0 * e_hue * vc.n_c * vc.ncb;
    const t = p1 * @sqrt(a * a + b * b) / (u + 0.305);
    const alpha_val = std.math.pow(f64, t, 0.9) *
        std.math.pow(f64, 1.64 - std.math.pow(f64, 0.29, vc.background_y_to_white_point_y), 0.73);
    const c_val = alpha_val * @sqrt(j / 100.0);
    const m = c_val * vc.fl_root;
    const s = 50.0 * @sqrt((alpha_val * vc.c) / (vc.aw + 4.0));
    const jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const mstar = 1.0 / 0.0228 * @log(1.0 + 0.0228 * m);
    const astar = mstar * @cos(hue_radians);
    const bstar = mstar * @sin(hue_radians);
    return Cam{ .hue = hue, .chroma = c_val, .j = j, .q = q, .m = m, .s = s, .jstar = jstar, .astar = astar, .bstar = bstar };
}

pub fn camFromInt(argb: Argb) Cam {
    return camFromIntAndViewingConditions(argb, kDefaultViewingConditions);
}
