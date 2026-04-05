const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseFast });

    const imageFlags = pkgFlags(b, &.{ "cairo", "libwebp", "libjpeg" });

    const imageModule = b.createModule(.{
        .root_source_file = b.path("../src/utils/image.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    imageModule.addIncludePath(.{ .cwd_relative = "/usr/include" });
    for (imageFlags.cflags) |f| {
        if (std.mem.startsWith(u8, f, "-I")) {
            imageModule.addIncludePath(.{ .cwd_relative = f[2..] });
        }
    }

    const exe = b.addExecutable(.{
        .name = "theme",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    exe.root_module.addImport("image", imageModule);
    applyPkgFlags(exe, imageFlags);
    exe.root_module.strip = true;
    b.installArtifact(exe);

    const run = b.addRunArtifact(exe);
    if (b.args) |args| for (args) |a| run.addArg(a);
    b.step("run", "Run theme").dependOn(&run.step);
}

fn pkgFlags(b: *std.Build, packages: []const []const u8) struct { cflags: [][]const u8, libs: [][]const u8 } {
    var cflagsArr = std.ArrayList([]const u8){};
    var libsArr = std.ArrayList([]const u8){};

    for (packages) |pkg| {
        const cflagsResult = b.run(&.{ "pkg-config", "--cflags", pkg });
        var it = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, cflagsResult, "\n"), ' ');
        while (it.next()) |f| cflagsArr.append(b.allocator, b.dupe(f)) catch {};

        const libsResult = b.run(&.{ "pkg-config", "--libs", pkg });
        var lit = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, libsResult, "\n"), ' ');
        while (lit.next()) |f| libsArr.append(b.allocator, b.dupe(f)) catch {};
    }

    return .{
        .cflags = cflagsArr.toOwnedSlice(b.allocator) catch &.{},
        .libs = libsArr.toOwnedSlice(b.allocator) catch &.{},
    };
}

fn applyPkgFlags(artifact: anytype, flags: anytype) void {
    artifact.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    artifact.addIncludePath(.{ .cwd_relative = "/usr/include" });
    for (flags.cflags) |f| {
        if (std.mem.startsWith(u8, f, "-I")) {
            artifact.addIncludePath(.{ .cwd_relative = f[2..] });
        }
    }
    for (flags.libs) |f| {
        if (std.mem.startsWith(u8, f, "-L")) {
            artifact.addLibraryPath(.{ .cwd_relative = f[2..] });
        } else if (std.mem.startsWith(u8, f, "-l")) {
            artifact.linkSystemLibrary(f[2..]);
        }
    }
}
