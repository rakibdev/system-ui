const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const gtkFlags = pkgFlags(b, &.{ "gtk+-3.0", "gtk-layer-shell-0", "cairo", "librsvg-2.0" });
    const imageFlags = pkgFlags(b, &.{ "libwebp", "libjpeg" });

    // Main CLI — compile as object, link with gcc to avoid sframe issue
    const exeObj = b.addLibrary(.{
        .name = "system-ui-obj",
        .linkage = .static,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    applyPkgFlags(exeObj, gtkFlags);

    var gccArgs = std.ArrayList([]const u8){};
    gccArgs.append(b.allocator, "gcc") catch {};
    gccArgs.append(b.allocator, "-o") catch {};
    const outPath = b.fmt("{s}/bin/system-ui", .{b.install_path});
    gccArgs.append(b.allocator, outPath) catch {};

    const gccLink = b.addSystemCommand(gccArgs.items);
    gccLink.addArtifactArg(exeObj);
    for (gtkFlags.libs) |l| gccLink.addArg(l);
    gccLink.addArg("-ldl");
    gccLink.addArg("-lm");
    gccLink.addArg("-lgcc");
    gccLink.addArg("-lgcc_s");
    gccLink.addArg("-lquadmath");
    gccLink.addArg("-L/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1");
    gccLink.step.dependOn(&exeObj.step);

    const installDir = b.addSystemCommand(&.{ "mkdir", "-p", b.fmt("{s}/bin", .{b.install_path}) });
    gccLink.step.dependOn(&installDir.step);

    b.getInstallStep().dependOn(&gccLink.step);

    const run_cmd = b.addSystemCommand(&.{outPath});
    run_cmd.step.dependOn(&gccLink.step);
    if (b.args) |args| for (args) |a| run_cmd.addArg(a);
    b.step("run", "Run system-ui").dependOn(&run_cmd.step);

    // Launcher .so extension — shared lib links fine with Zig's linker
    const launcher = b.addLibrary(.{
        .name = "launcher",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .root_source_file = b.path("extensions/launcher/main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    applyPkgFlags(launcher, gtkFlags);
    applyPkgFlags(launcher, imageFlags);
    launcher.addCSourceFile(.{ .file = b.path("src/gtk_shim.c"), .flags = &.{} });
    b.installArtifact(launcher);
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
