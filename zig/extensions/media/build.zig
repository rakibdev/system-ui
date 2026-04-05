const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseFast });

    const gioFlags = pkgFlags(b, &.{"libsystemd"});

    const mediaModule = b.createModule(.{
        .root_source_file = b.path("../../src/services/media.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    mediaModule.addIncludePath(.{ .cwd_relative = "/usr/include" });
    for (gioFlags.cflags) |f| {
        if (std.mem.startsWith(u8, f, "-I")) mediaModule.addIncludePath(.{ .cwd_relative = f[2..] });
    }
    mediaModule.addObjectFile(.{ .cwd_relative = "/usr/lib/libsystemd.so" });

    const logModule = b.createModule(.{
        .root_source_file = b.path("../../src/utils/log.zig"),
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "media",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    exe.root_module.addImport("media", mediaModule);
    exe.root_module.addImport("log", logModule);
    applyPkgFlags(exe, gioFlags);
    exe.root_module.linkSystemLibrary("systemd", .{});
    exe.root_module.strip = true;
    b.installArtifact(exe);

    const run = b.addRunArtifact(exe);
    if (b.args) |args| for (args) |a| run.addArg(a);
    b.step("run", "Run media").dependOn(&run.step);
}

fn pkgFlags(b: *std.Build, packages: []const []const u8) struct { cflags: [][]const u8, libs: [][]const u8 } {
    var cflags = std.ArrayList([]const u8){};
    var libs = std.ArrayList([]const u8){};
    for (packages) |pkg| {
        var it = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, b.run(&.{ "pkg-config", "--cflags", pkg }), "\n"), ' ');
        while (it.next()) |f| cflags.append(b.allocator, b.dupe(f)) catch {};
        var lit = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, b.run(&.{ "pkg-config", "--libs", pkg }), "\n"), ' ');
        while (lit.next()) |f| libs.append(b.allocator, b.dupe(f)) catch {};
    }
    return .{
        .cflags = cflags.toOwnedSlice(b.allocator) catch &.{},
        .libs = libs.toOwnedSlice(b.allocator) catch &.{},
    };
}

fn applyPkgFlags(artifact: anytype, flags: anytype) void {
    artifact.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    artifact.addIncludePath(.{ .cwd_relative = "/usr/include" });
    for (flags.cflags) |f| {
        if (std.mem.startsWith(u8, f, "-I")) artifact.addIncludePath(.{ .cwd_relative = f[2..] });
    }
    for (flags.libs) |f| {
        if (std.mem.startsWith(u8, f, "-L")) {
            artifact.addLibraryPath(.{ .cwd_relative = f[2..] });
        } else if (std.mem.startsWith(u8, f, "-l")) {
            artifact.linkSystemLibrary(f[2..]);
        }
    }
}
