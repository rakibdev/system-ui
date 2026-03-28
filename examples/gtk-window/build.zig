const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Compile Zig to object, link with gcc to avoid sframe issue
    const obj = b.addObject(.{
        .name = "gtk-example-obj",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    const pkgConfigCflags = b.run(&.{ "pkg-config", "--cflags", "gtk+-3.0" });
    const pkgConfigLibs = b.run(&.{ "pkg-config", "--libs", "gtk+-3.0" });

    var cflagsIt = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, pkgConfigCflags, "\n"), ' ');
    while (cflagsIt.next()) |f| {
        if (std.mem.startsWith(u8, f, "-I")) {
            obj.addIncludePath(.{ .cwd_relative = f[2..] });
        }
    }

    const outPath = b.fmt("{s}/bin/gtk-example", .{b.install_path});

    var gccArgs = std.ArrayList([]const u8){};
    gccArgs.append(b.allocator, "gcc") catch {};
    gccArgs.append(b.allocator, "-o") catch {};
    gccArgs.append(b.allocator, outPath) catch {};

    const gccLink = b.addSystemCommand(gccArgs.items);
    gccLink.addArtifactArg(obj);

    var libsIt = std.mem.tokenizeScalar(u8, std.mem.trimRight(u8, pkgConfigLibs, "\n"), ' ');
    while (libsIt.next()) |f| gccLink.addArg(f);

    gccLink.addArg("-ldl");
    gccLink.addArg("-lm");

    const installDir = b.addSystemCommand(&.{ "mkdir", "-p", b.fmt("{s}/bin", .{b.install_path}) });
    gccLink.step.dependOn(&installDir.step);
    gccLink.step.dependOn(&obj.step);

    b.getInstallStep().dependOn(&gccLink.step);

    const run = b.addSystemCommand(&.{outPath});
    run.step.dependOn(&gccLink.step);
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Run the app").dependOn(&run.step);
}
