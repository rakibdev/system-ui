const std = @import("std");
const qt6 = @import("libqt6zig");
const qapplication = qt6.qapplication;
const qpushbutton = qt6.qpushbutton;
const qwidget = qt6.qwidget;
const qlabel = qt6.qlabel;
const qboxlayout = qt6.qboxlayout;
const qboxlayout_enums = qt6.qboxlayout_enums;

var counter: isize = 0;
var label: ?*anyopaque = null;

pub fn main() void {
    const argc = std.os.argv.len;
    const argv = std.os.argv.ptr;

    const qapp = qapplication.New(argc, argv);
    defer qapplication.Delete(qapp);

    const window = qwidget.New2();
    if (window == null) @panic("Failed to create window");
    defer qwidget.Delete(window);

    qwidget.SetWindowTitle(window, "Qt Zig Window");
    qwidget.Resize(window, 400, 200);

    const layout = qboxlayout.New2(qboxlayout_enums.Direction.TopToBottom, window);

    label = qlabel.New3("Hello from Zig + Qt6!");
    qlabel.SetAlignment(label, 0x84);
    qboxlayout.AddWidget(layout, label);

    const button = qpushbutton.New5("Click Me!", window);
    qpushbutton.OnClicked(button, onButtonClicked);
    qboxlayout.AddWidget(layout, button);

    qwidget.SetLayout(window, layout);
    qwidget.Show(window);

    _ = qapplication.Exec();
}

fn onButtonClicked(_: ?*anyopaque) callconv(.c) void {
    counter += 1;
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrintZ(&buffer, "Clicked {d} time(s)", .{counter}) catch return;
    qlabel.SetText(label, text);
}
