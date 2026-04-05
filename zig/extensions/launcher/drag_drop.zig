const std = @import("std");
const c = @import("gtk_extern.zig");
const main = @import("main.zig");

const Launcher = main.Launcher;
const App = main.App;
const AppClickCtx = main.AppClickCtx;

const targetName = "application/x-pinned-app";


pub fn setupDropTargets(launcher: *Launcher) void {
    const pinGrid = launcher.pinGrid orelse return;
    const grid = launcher.grid orelse return;

    // GTK_DEST_DEFAULT_DROP|HIGHLIGHT without ALL: we must call gdk_drag_status
    // ourselves in drag-motion (via shim) so GTK commits the drop. Using
    // GTK_DEST_DEFAULT_ALL silently rejects drops when no motion handler calls it.
    c.launcher_drag_dest_set(pinGrid, targetName, 0, c.GDK_ACTION_MOVE);
    _ = c.g_signal_connect_data(pinGrid, "drag-motion", @ptrCast(&c.launcher_drag_motion), null, null, 0);
    _ = c.g_signal_connect_data(pinGrid, "drag-data-received", @ptrCast(&onPinGridDropReceive), launcher, null, 0);

    c.launcher_drag_dest_set(grid, targetName, 0, c.GDK_ACTION_MOVE);
    _ = c.g_signal_connect_data(grid, "drag-motion", @ptrCast(&c.launcher_drag_motion), null, null, 0);
    _ = c.g_signal_connect_data(grid, "drag-data-received", @ptrCast(&onAppGridDropReceive), launcher, null, 0);
}

pub fn setupSource(launcher: *Launcher, evBox: *c.GtkWidget, appIdx: usize) void {
    c.launcher_drag_source_set(evBox, @intFromEnum(c.GDK_BUTTON1_MASK), targetName, 0, c.GDK_ACTION_MOVE);

    const ctx = launcher.alloc.create(AppClickCtx) catch return;
    ctx.* = .{ .launcher = launcher, .appIdx = appIdx };
    _ = c.g_signal_connect_data(evBox, "drag-begin", @ptrCast(&onDragBegin), ctx, null, 0);
    _ = c.g_signal_connect_data(evBox, "drag-end", @ptrCast(&onDragEnd), ctx, null, 0);
    _ = c.g_signal_connect_data(evBox, "drag-data-get", @ptrCast(&onDragDataGet), ctx, null, 0);
}

fn onDragBegin(_: *c.GtkWidget, ctx: *c.GdkDragContext, data: *AppClickCtx) callconv(.{ .x86_64_sysv = .{} }) void {
    const app = &data.launcher.apps.items[data.appIdx];
    if (app.element) |fbc| {
        c.gtk_widget_set_state_flags(fbc, c.GTK_STATE_FLAG_ACTIVE, 0);
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(fbc), "dragging");
    }
    if (app.data.icon.len > 0) {
        const dragIcon = c.gtk_box_new(c.GTK_ORIENTATION_VERTICAL, 0).?;
        c.gtk_widget_set_size_request(dragIcon, 40, 40);
        var cssBuf: [1024]u8 = undefined;
        const css = std.fmt.bufPrintZ(&cssBuf, ".drag-icon {{ background-image: url('{s}'); }}", .{app.data.icon}) catch "";
        if (css.len > 0) {
            const provider = c.gtk_css_provider_new();
            _ = c.gtk_css_provider_load_from_data(provider, css, -1, null);
            c.gtk_style_context_add_provider(c.gtk_widget_get_style_context(dragIcon), @ptrCast(provider), c.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            c.gtk_style_context_add_class(c.gtk_widget_get_style_context(dragIcon), "drag-icon");
            c.g_object_unref(provider);
        }
        _ = c.gtk_widget_show_all(dragIcon);
        c.gtk_drag_set_icon_widget(ctx, dragIcon, 20, 20);
    } else {
        c.gtk_drag_set_icon_default(ctx);
    }
}

fn onDragEnd(_: *c.GtkWidget, _: *c.GdkDragContext, data: *AppClickCtx) callconv(.{ .x86_64_sysv = .{} }) void {
    const app = &data.launcher.apps.items[data.appIdx];
    if (app.element) |fbc| {
        c.gtk_widget_unset_state_flags(fbc, c.GTK_STATE_FLAG_ACTIVE);
        c.gtk_style_context_remove_class(c.gtk_widget_get_style_context(fbc), "dragging");
    }
}

fn onDragDataGet(_: *c.GtkWidget, _: *c.GdkDragContext, sel: *c.GtkSelectionData, _: c_uint, _: c_uint, data: *AppClickCtx) callconv(.{ .x86_64_sysv = .{} }) void {
    const app = &data.launcher.apps.items[data.appIdx];
    const basename = std.fs.path.basename(app.data.file);
    c.launcher_drag_data_set_filename(sel, basename.ptr, @intCast(basename.len));
}

fn onPinGridDropReceive(widget: *c.GtkWidget, ctx: *c.GdkDragContext, x: c_int, y: c_int, sel: *c.GtkSelectionData, _: c_uint, time: c_uint, launcher: *Launcher) callconv(.{ .x86_64_sysv = .{} }) void {
    const len = c.gtk_selection_data_get_length(sel);
    if (len <= 0) { c.gtk_drag_finish(ctx, 0, 0, time); return; }

    const rawData = c.gtk_selection_data_get_data(sel) orelse { c.gtk_drag_finish(ctx, 0, 0, time); return; };
    const filename = rawData[0..@intCast(len)];

    const childAtPos = c.gtk_flow_box_get_child_at_pos(@ptrCast(widget), x, y);
    const dropIndex: usize = if (childAtPos) |cap| @intCast(c.gtk_flow_box_child_get_index(cap)) else launcher.pinnedApps.items.len;

    const filenameStr = launcher.alloc.dupe(u8, filename) catch { c.gtk_drag_finish(ctx, 0, 0, time); return; };
    defer launcher.alloc.free(filenameStr);

    if (main.pinnedHas(launcher, filenameStr)) {
        main.pinnedReorder(launcher, filenameStr, dropIndex);
    } else {
        main.pinnedInsertAt(launcher, filenameStr, dropIndex);
    }

    c.gtk_drag_finish(ctx, 1, 0, time);
    main.updateGrid(launcher, true);
}

fn onAppGridDropReceive(widget: *c.GtkWidget, ctx: *c.GdkDragContext, _: c_int, _: c_int, sel: *c.GtkSelectionData, _: c_uint, time: c_uint, launcher: *Launcher) callconv(.{ .x86_64_sysv = .{} }) void {
    _ = widget;
    const len = c.gtk_selection_data_get_length(sel);
    if (len <= 0) { c.gtk_drag_finish(ctx, 0, 0, time); return; }

    const rawData = c.gtk_selection_data_get_data(sel) orelse { c.gtk_drag_finish(ctx, 0, 0, time); return; };
    const filename = rawData[0..@intCast(len)];
    const filenameStr = launcher.alloc.dupe(u8, filename) catch { c.gtk_drag_finish(ctx, 0, 0, time); return; };
    defer launcher.alloc.free(filenameStr);

    if (main.pinnedHas(launcher, filenameStr)) {
        main.pinnedToggle(launcher, filenameStr);
    }
    c.gtk_drag_finish(ctx, 1, 0, time);
    main.updateGrid(launcher, true);
}
