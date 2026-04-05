const std = @import("std");
const utils = @import("utils.zig");
const viewing_conditions = @import("viewing_conditions.zig");

const Argb = utils.Argb;
const Vec3 = utils.Vec3;
const kDefaultViewingConditions = viewing_conditions.kDefaultViewingConditions;

const kScaledDiscountFromLinrgb = [3][3]f64{
    .{ 0.001200833568784504, 0.002389694492170889, 0.0002795742885861124 },
    .{ 0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398 },
    .{ 0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076 },
};

const kLinrgbFromScaledDiscount = [3][3]f64{
    .{ 1373.2198709594231, -1100.4251190754821, -7.278681089101213 },
    .{ -271.815969077903, 559.6580465940733, -32.46047482791194 },
    .{ 1.9622899599665666, -57.173814538844006, 308.7233197812385 },
};

const kYFromLinrgb = [3]f64{ 0.2126, 0.7152, 0.0722 };

const kCriticalPlanes = [255]f64{
    0.015176349177441876, 0.045529047532325624, 0.07588174588720938,
    0.10623444424209313,  0.13658714259697685,  0.16693984095186062,
    0.19729253930674434,  0.2276452376616281,   0.2579979360165119,
    0.28835063437139563,  0.3188300904430532,   0.350925934958123,
    0.3848314933096426,   0.42057480301049466,  0.458183274052838,
    0.4976837250274023,   0.5391024159806381,   0.5824650784040898,
    0.6277969426914107,   0.6751227633498623,   0.7244668422128921,
    0.775853049866786,    0.829304845476233,    0.8848452951698498,
    0.942497089126609,    1.0022825574869039,   1.0642236851973577,
    1.1283421258858297,   1.1946592148522128,   1.2631959812511864,
    1.3339731595349034,   1.407011200216447,    1.4823302800086415,
    1.5599503113873272,   1.6398909516233677,   1.7221716113234105,
    1.8068114625156377,   1.8938294463134073,   1.9832442801866852,
    2.075074464868551,    2.1693382909216234,   2.2660538449872063,
    2.36523901573795,     2.4669114995532007,   2.5710888059345764,
    2.6777882626779785,   2.7870270208169257,   2.898822059350997,
    3.0131901897720907,   3.1301480604002863,   3.2497121605402226,
    3.3718988244681087,   3.4967242352587946,   3.624204428461639,
    3.754355295633311,    3.887192587735158,    4.022731918402185,
    4.160988767090289,    4.301978482107941,    4.445716283538092,
    4.592217266055746,    4.741496401646282,    4.893568542229298,
    5.048448422192488,    5.20615066083972,     5.3666897647573375,
    5.5300801301023865,   5.696336044816294,    5.865471690767354,
    6.037501145825082,    6.212438385869475,    6.390297286737924,
    6.571091626112461,    6.7548350853498045,   6.941541251256611,
    7.131223617812143,    7.323895587840543,    7.5195704746346665,
    7.7182615035334345,   7.919981813454504,    8.124744458384042,
    8.332562408825165,    8.543448553206703,    8.757415699253682,
    8.974476575321063,    9.194643831691977,    9.417930041841839,
    9.644347703669503,    9.873909240696694,    10.106627003236781,
    10.342513269534024,   10.58158024687427,    10.8238400726681,
    11.069304815507364,   11.317986476196008,   11.569896988756009,
    11.825048221409341,   12.083451977536606,   12.345119996613247,
    12.610063955123938,   12.878295467455942,   13.149826086772048,
    13.42466730586372,    13.702830557985108,   13.984327217668513,
    14.269168601521828,   14.55736596900856,    14.848930523210871,
    15.143873411576273,   15.44220572664832,    15.743938506781891,
    16.04908273684337,    16.35764934889634,    16.66964922287304,
    16.985093187232053,   17.30399201960269,    17.62635644741625,
    17.95219714852476,    18.281524751807332,   18.614349837764564,
    18.95068293910138,    19.290534541298456,   19.633915083172692,
    19.98083495742689,    20.331304511189067,   20.685334046541502,
    21.042933821039977,   21.404114048223256,   21.76888489811322,
    22.137256497705877,   22.50923893145328,    22.884842241736916,
    23.264076429332462,   23.6469514538663,     24.033477234264016,
    24.42366364919083,    24.817520537484558,   25.21505769858089,
    25.61628489293138,    26.021211842414342,   26.429848230738664,
    26.842203703840827,   27.258287870275353,   27.678110301598522,
    28.10168053274597,    28.529008062403893,   28.96010235337422,
    29.39497283293396,    29.83362889318845,    30.276079891419332,
    30.722335150426627,   31.172403958865512,   31.62629557157785,
    32.08401920991837,    32.54558406207592,    33.010999283389665,
    33.4802739966603,     33.953417292456834,   34.430438229418264,
    34.911345834551085,   35.39614910352207,    35.88485700094671,
    36.37747846067349,    36.87402238606382,    37.37449765026789,
    37.87891309649659,    38.38727753828926,    38.89959975977785,
    39.41588851594697,    39.93615253289054,    40.460400508064545,
    40.98864111053629,    41.520882981230194,   42.05713473317016,
    42.597404951718396,   43.141702194811224,   43.6900349931913,
    44.24241185063697,    44.798841244188324,   45.35933162437017,
    45.92389141541209,    46.49252901546552,    47.065252796817916,
    47.64207110610409,    48.22299226451468,    48.808024568002054,
    49.3971762874833,     49.9904556690408,     50.587870934119984,
    51.189430279724725,   51.79514187861014,    52.40501387947288,
    53.0190544071392,     53.637271562750364,   54.259673423945976,
    54.88626804504493,    55.517063457223934,   56.15206766869424,
    56.79128866487574,    57.43473440856916,    58.08241284012621,
    58.734331877617365,   59.39049941699807,    60.05092333227251,
    60.715611475655585,   61.38457167773311,    62.057811747619894,
    62.7353394731159,     63.417162620860914,   64.10328893648692,
    64.79372614476921,    65.48848194977529,    66.18756403501224,
    66.89098006357258,    67.59873767827808,    68.31084450182222,
    69.02730813691093,    69.74813616640164,    70.47333615344107,
    71.20291564160104,    71.93688215501312,    72.67524319850172,
    73.41800625771542,    74.16517879925733,    74.9167682708136,
    75.67278210128072,    76.43322770089146,    77.1981124613393,
    77.96744375590167,    78.74122893956174,    79.51947534912904,
    80.30219030335869,    81.08938110306934,    81.88105503125999,
    82.67721935322541,    83.4778813166706,     84.28304815182372,
    85.09272707154808,    85.90692527145302,    86.72564993000343,
    87.54890820862819,    88.3767072518277,     89.2090541872801,
    90.04595612594655,    90.88742016217518,    91.73345337380438,
    92.58406282226491,    93.43925555268066,    94.29903859396902,
    95.16341895893969,    96.03240364439274,    96.9059996312159,
    97.78421388448044,    98.6670533535366,     99.55452497210776,
};

fn sanitizeRadians(angle: f64) f64 {
    return @mod(angle + std.math.pi * 8.0, std.math.pi * 2.0);
}

fn trueDelinearized(rgb_component: f64) f64 {
    const normalized = rgb_component / 100.0;
    const dl: f64 = if (normalized <= 0.0031308)
        normalized * 12.92
    else
        1.055 * std.math.pow(f64, normalized, 1.0 / 2.4) - 0.055;
    return dl * 255.0;
}

fn chromaticAdaptation(component: f64) f64 {
    const af = std.math.pow(f64, @abs(component), 0.42);
    return utils.signum(component) * 400.0 * af / (af + 27.13);
}

fn hueOf(linrgb: Vec3) f64 {
    const scaled = utils.matrixMultiply(linrgb, kScaledDiscountFromLinrgb);
    const r_a = chromaticAdaptation(scaled.a);
    const g_a = chromaticAdaptation(scaled.b);
    const b_a = chromaticAdaptation(scaled.c);
    const a = (11.0 * r_a - 12.0 * g_a + b_a) / 11.0;
    const b = (r_a + g_a - 2.0 * b_a) / 9.0;
    return std.math.atan2(b, a);
}

fn areInCyclicOrder(a: f64, b: f64, c: f64) bool {
    return sanitizeRadians(b - a) < sanitizeRadians(c - a);
}

fn intercept(source: f64, mid: f64, target: f64) f64 {
    return (mid - source) / (target - source);
}

fn lerpPoint(source: Vec3, t: f64, target: Vec3) Vec3 {
    return Vec3{
        .a = source.a + (target.a - source.a) * t,
        .b = source.b + (target.b - source.b) * t,
        .c = source.c + (target.c - source.c) * t,
    };
}

fn getAxis(v: Vec3, axis: usize) f64 {
    return switch (axis) {
        0 => v.a,
        1 => v.b,
        2 => v.c,
        else => -1.0,
    };
}

fn setCoordinate(source: Vec3, coordinate: f64, target: Vec3, axis: usize) Vec3 {
    const t = intercept(getAxis(source, axis), coordinate, getAxis(target, axis));
    return lerpPoint(source, t, target);
}

fn isBounded(x: f64) bool {
    return 0.0 <= x and x <= 100.0;
}

fn nthVertex(y: f64, n: usize) Vec3 {
    const k_r = kYFromLinrgb[0];
    const k_g = kYFromLinrgb[1];
    const k_b = kYFromLinrgb[2];
    const coord_a: f64 = if (n % 4 <= 1) 0.0 else 100.0;
    const coord_b: f64 = if (n % 2 == 0) 0.0 else 100.0;
    const invalid = Vec3{ .a = -1.0, .b = -1.0, .c = -1.0 };
    if (n < 4) {
        const g = coord_a;
        const b = coord_b;
        const r = (y - g * k_g - b * k_b) / k_r;
        return if (isBounded(r)) Vec3{ .a = r, .b = g, .c = b } else invalid;
    } else if (n < 8) {
        const b = coord_a;
        const r = coord_b;
        const g = (y - r * k_r - b * k_b) / k_g;
        return if (isBounded(g)) Vec3{ .a = r, .b = g, .c = b } else invalid;
    } else {
        const r = coord_a;
        const g = coord_b;
        const b = (y - r * k_r - g * k_g) / k_b;
        return if (isBounded(b)) Vec3{ .a = r, .b = g, .c = b } else invalid;
    }
}

fn bisectToSegment(y: f64, target_hue: f64) [2]Vec3 {
    const invalid = Vec3{ .a = -1.0, .b = -1.0, .c = -1.0 };
    var left = invalid;
    var right = invalid;
    var left_hue: f64 = 0;
    var right_hue: f64 = 0;
    var initialized = false;
    var uncut = true;
    for (0..12) |n| {
        const mid = nthVertex(y, n);
        if (mid.a < 0) continue;
        const mid_hue = hueOf(mid);
        if (!initialized) {
            left = mid;
            right = mid;
            left_hue = mid_hue;
            right_hue = mid_hue;
            initialized = true;
            continue;
        }
        if (uncut or areInCyclicOrder(left_hue, mid_hue, right_hue)) {
            uncut = false;
            if (areInCyclicOrder(left_hue, target_hue, mid_hue)) {
                right = mid;
                right_hue = mid_hue;
            } else {
                left = mid;
                left_hue = mid_hue;
            }
        }
    }
    return .{ left, right };
}

fn midpoint(a: Vec3, b: Vec3) Vec3 {
    return Vec3{ .a = (a.a + b.a) / 2.0, .b = (a.b + b.b) / 2.0, .c = (a.c + b.c) / 2.0 };
}

fn criticalPlaneBelow(x: f64) i32 {
    return @intFromFloat(@floor(x - 0.5));
}

fn criticalPlaneAbove(x: f64) i32 {
    return @intFromFloat(@ceil(x - 0.5));
}

fn bisectToLimit(y: f64, target_hue: f64) Vec3 {
    const seg = bisectToSegment(y, target_hue);
    var left = seg[0];
    var left_hue = hueOf(left);
    var right = seg[1];
    for (0..3) |axis| {
        if (getAxis(left, axis) != getAxis(right, axis)) {
            var l_plane: i32 = -1;
            var r_plane: i32 = 255;
            if (getAxis(left, axis) < getAxis(right, axis)) {
                l_plane = criticalPlaneBelow(trueDelinearized(getAxis(left, axis)));
                r_plane = criticalPlaneAbove(trueDelinearized(getAxis(right, axis)));
            } else {
                l_plane = criticalPlaneAbove(trueDelinearized(getAxis(left, axis)));
                r_plane = criticalPlaneBelow(trueDelinearized(getAxis(right, axis)));
            }
            for (0..8) |_| {
                if (@abs(r_plane - l_plane) <= 1) break;
                const m_plane: i32 = @intFromFloat(@floor(@as(f64, @floatFromInt(l_plane + r_plane)) / 2.0));
                const mid_plane_coordinate = kCriticalPlanes[@intCast(m_plane)];
                const mid = setCoordinate(left, mid_plane_coordinate, right, axis);
                const mid_hue = hueOf(mid);
                if (areInCyclicOrder(left_hue, target_hue, mid_hue)) {
                    right = mid;
                    r_plane = m_plane;
                } else {
                    left = mid;
                    left_hue = mid_hue;
                    l_plane = m_plane;
                }
            }
        }
    }
    return midpoint(left, right);
}

fn inverseChromaticAdaptation(adapted: f64) f64 {
    const adapted_abs = @abs(adapted);
    const base = @max(0.0, 27.13 * adapted_abs / (400.0 - adapted_abs));
    return utils.signum(adapted) * std.math.pow(f64, base, 1.0 / 0.42);
}

fn findResultByJ(hue_radians: f64, chroma: f64, y: f64) Argb {
    var j = @sqrt(y) * 11.0;
    const vc = kDefaultViewingConditions;
    const t_inner_coeff = 1.0 / std.math.pow(f64, 1.64 - std.math.pow(f64, 0.29, vc.background_y_to_white_point_y), 0.73);
    const e_hue = 0.25 * (@cos(hue_radians + 2.0) + 3.8);
    const p1 = e_hue * (50000.0 / 13.0) * vc.n_c * vc.ncb;
    const h_sin = @sin(hue_radians);
    const h_cos = @cos(hue_radians);
    var i: usize = 0;
    while (i < 5) : (i += 1) {
        const j_normalized = j / 100.0;
        const alpha: f64 = if (chroma == 0.0 or j == 0.0) 0.0 else chroma / @sqrt(j_normalized);
        const t = std.math.pow(f64, alpha * t_inner_coeff, 1.0 / 0.9);
        const ac = vc.aw * std.math.pow(f64, j_normalized, 1.0 / vc.c / vc.z);
        const p2 = ac / vc.nbb;
        const gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * h_cos + 108.0 * t * h_sin);
        const a = gamma * h_cos;
        const b = gamma * h_sin;
        const r_a = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
        const g_a = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
        const b_a = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;
        const scaled = Vec3{
            .a = inverseChromaticAdaptation(r_a),
            .b = inverseChromaticAdaptation(g_a),
            .c = inverseChromaticAdaptation(b_a),
        };
        const linrgb = utils.matrixMultiply(scaled, kLinrgbFromScaledDiscount);
        if (linrgb.a < 0 or linrgb.b < 0 or linrgb.c < 0) return 0;
        const fnj = kYFromLinrgb[0] * linrgb.a + kYFromLinrgb[1] * linrgb.b + kYFromLinrgb[2] * linrgb.c;
        if (fnj <= 0) return 0;
        if (i == 4 or @abs(fnj - y) < 0.002) {
            if (linrgb.a > 100.01 or linrgb.b > 100.01 or linrgb.c > 100.01) return 0;
            return utils.argbFromLinrgb(linrgb);
        }
        j = j - (fnj - y) * j / (2.0 * fnj);
    }
    return 0;
}

pub fn solveToInt(hue_degrees: f64, chroma: f64, lstar: f64) Argb {
    if (chroma < 0.0001 or lstar < 0.0001 or lstar > 99.9999) {
        return utils.intFromLstar(lstar);
    }
    const hue = utils.sanitizeDegreesDouble(hue_degrees);
    const hue_radians = hue / 180.0 * std.math.pi;
    const y = utils.yFromLstar(lstar);
    const exact_answer = findResultByJ(hue_radians, chroma, y);
    if (exact_answer != 0) return exact_answer;
    const linrgb = bisectToLimit(y, hue_radians);
    return utils.argbFromLinrgb(linrgb);
}
