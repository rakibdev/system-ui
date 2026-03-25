const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const qt6zig = b.dependency("libqt6zig", .{
        .target = target,
        .optimize = optimize,
    });

    // Compile Zig to object, link with g++ to avoid sframe issue
    const obj = b.addObject(.{
        .name = "qt-example-obj",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    obj.root_module.addImport("libqt6zig", qt6zig.module("libqt6zig"));
    obj.addIncludePath(.{ .cwd_relative = "/usr/include/qt6" });
    obj.addIncludePath(.{ .cwd_relative = "/usr/include/qt6/QtCore" });
    obj.addIncludePath(.{ .cwd_relative = "/usr/include/qt6/QtGui" });
    obj.addIncludePath(.{ .cwd_relative = "/usr/include/qt6/QtWidgets" });

    const outPath = b.fmt("{s}/bin/qt-example", .{b.install_path});

    var gppArgs = std.ArrayList([]const u8){};
    gppArgs.append(b.allocator, "g++") catch {};
    gppArgs.append(b.allocator, "-o") catch {};
    gppArgs.append(b.allocator, outPath) catch {};

    const gppLink = b.addSystemCommand(gppArgs.items);
    gppLink.addArtifactArg(obj);

    // Link qt6zig static libs
    gppLink.addArtifactArg(qt6zig.artifact("qapplication"));
    gppLink.addArtifactArg(qt6zig.artifact("qabstractbutton"));
    gppLink.addArtifactArg(qt6zig.artifact("qpushbutton"));
    gppLink.addArtifactArg(qt6zig.artifact("qwidget"));
    gppLink.addArtifactArg(qt6zig.artifact("qlabel"));
    gppLink.addArtifactArg(qt6zig.artifact("qboxlayout"));

    // Link Qt6 system libs
    gppLink.addArg("-lQt6Core");
    gppLink.addArg("-lQt6Gui");
    gppLink.addArg("-lQt6Widgets");
    gppLink.addArg("-lstdc++");

    const installDir = b.addSystemCommand(&.{ "mkdir", "-p", b.fmt("{s}/bin", .{b.install_path}) });
    gppLink.step.dependOn(&installDir.step);
    gppLink.step.dependOn(&obj.step);

    b.getInstallStep().dependOn(&gppLink.step);

    const run = b.addSystemCommand(&.{outPath});
    run.step.dependOn(&gppLink.step);
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Run the app").dependOn(&run.step);
}
