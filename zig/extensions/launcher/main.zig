const std = @import("std");
const c = @import("gtk_extern.zig");
const dragDrop = @import("drag_drop.zig");

const CResponse = extern struct {
    content: [*:0]const u8,
    status: u8,
};

const Extension = extern struct {
    handle: ?*anyopaque,
    onRequestFn: *const fn (*Extension, [*:0]const u8) callconv(.{ .x86_64_sysv = .{} }) CResponse,
    deinitFn: *const fn (*Extension) callconv(.{ .x86_64_sysv = .{} }) void,
};

const HOME = std.posix.getenv("HOME") orelse "/root";
const CONFIG_DIR = blk: {
    break :blk "";
};

const AppAction = struct {
    label: []const u8 = "",
    exec: []const u8 = "",
};

pub const AppData = struct {
    file: []const u8 = "",
    label: []const u8 = "",
    exec: []const u8 = "",
    icon: []const u8 = "",
    isCircular: bool = false,
};

pub const App = struct {
    data: AppData,
    element: ?*c.GtkWidget = null, // FlowBoxChild widget ptr
};

const LauncherConfig = struct {
    pinnedApps: [][]const u8 = &.{},
};

const AppCache = struct {
    apps: []AppData = &.{},
    updatedAt: []const u8 = "",
};

pub const Launcher = struct {
    ext: Extension,
    alloc: std.mem.Allocator,

    window: ?*c.GtkWidget = null,
    menu: ?*c.GtkWidget = null,
    search: ?*c.GtkWidget = null,
    searchPlaceholder: ?*c.GtkWidget = null,
    pinGrid: ?*c.GtkWidget = null,
    grid: ?*c.GtkWidget = null,

    apps: std.ArrayList(App),
    configPath: []const u8,
    cachePath: []const u8,
    userCssPath: []const u8,
    extDir: []const u8,

    pinnedApps: std.ArrayList([]const u8),
};

fn isIconCircular(path: []const u8) bool {
    if (path.len == 0) return false;
    var pbuf: [4096]u8 = undefined;
    const pathZ = std.fmt.bufPrintZ(&pbuf, "{s}", .{path}) catch return false;

    const ext_start = std.mem.lastIndexOfScalar(u8, path, '.') orelse return false;
    const ext = path[ext_start..];

    var surface: ?*c.cairo_surface_t = null;
    if (std.ascii.eqlIgnoreCase(ext, ".png")) {
        surface = c.cairo_image_surface_create_from_png(pathZ);
    } else if (std.ascii.eqlIgnoreCase(ext, ".svg")) {
        surface = c.shim_create_svg_surface(pathZ, 48, 48);
    } else if (std.ascii.eqlIgnoreCase(ext, ".webp")) {
        surface = c.shim_create_webp_surface(pathZ);
    } else if (std.ascii.eqlIgnoreCase(ext, ".jpg") or std.ascii.eqlIgnoreCase(ext, ".jpeg")) {
        surface = c.shim_create_jpeg_surface(pathZ);
    } else return false;

    const surf = surface orelse return false;
    defer c.cairo_surface_destroy(surf);

    if (c.cairo_surface_status(surf) != c.CAIRO_STATUS_SUCCESS) return false;

    const width = c.cairo_image_surface_get_width(surf);
    const height = c.cairo_image_surface_get_height(surf);
    if (width < 16 or height < 16) return false;

    const dataOpt = c.cairo_image_surface_get_data(surf);
    if (dataOpt == null) return false;
    const data: [*]u8 = dataOpt.?;
    const stride = c.cairo_image_surface_get_stride(surf);

    const cx: f64 = @as(f64, @floatFromInt(width)) / 2.0;
    const cy: f64 = @as(f64, @floatFromInt(height)) / 2.0;
    const radius: f64 = @as(f64, @floatFromInt(@min(width, height))) / 2.0 - 2.0;

    var edgePixels: i32 = 0;
    var circularPixels: i32 = 0;
    var totalTransparentOutside: i32 = 0;
    var totalOpaqueInside: i32 = 0;
    var samplesOutside: i32 = 0;
    var samplesInside: i32 = 0;

    var y: i32 = 0;
    while (y < height) : (y += 1) {
        var x: i32 = 0;
        while (x < width) : (x += 1) {
            const dx: f64 = @as(f64, @floatFromInt(x)) - cx;
            const dy: f64 = @as(f64, @floatFromInt(y)) - cy;
            const dist = @sqrt(dx * dx + dy * dy);

            const offset = @as(usize, @intCast(y)) * @as(usize, @intCast(stride)) + @as(usize, @intCast(x)) * 4;
            const pixel = data[offset .. offset + 4];
            const alpha = pixel[3];

            const nearEdge = dist >= radius - 3 and dist <= radius + 3;
            if (nearEdge) {
                edgePixels += 1;
                if ((dist <= radius and alpha > 128) or (dist > radius and alpha <= 128))
                    circularPixels += 1;
            }
            if (dist < radius - 5) {
                samplesInside += 1;
                if (alpha > 128) totalOpaqueInside += 1;
            } else if (dist > radius + 5) {
                samplesOutside += 1;
                if (alpha <= 128) totalTransparentOutside += 1;
            }
        }
    }

    if (edgePixels == 0) return false;

    const circularRatio: f64 = @as(f64, @floatFromInt(circularPixels)) / @as(f64, @floatFromInt(edgePixels));
    const insideRatio: f64 = if (samplesInside > 0) @as(f64, @floatFromInt(totalOpaqueInside)) / @as(f64, @floatFromInt(samplesInside)) else 0;
    const outsideRatio: f64 = if (samplesOutside > 0) @as(f64, @floatFromInt(totalTransparentOutside)) / @as(f64, @floatFromInt(samplesOutside)) else 0;

    return circularRatio > 0.7 and insideRatio > 0.6 and outsideRatio > 0.85;
}

fn resolveIconPath(alloc: std.mem.Allocator, iconName: []const u8) []const u8 {
    var buf: [512]u8 = undefined;
    const z = std.fmt.bufPrintZ(&buf, "{s}", .{iconName}) catch return "";
    const info = c.gtk_icon_theme_lookup_icon(c.gtk_icon_theme_get_default(), z, 48, c.GTK_ICON_LOOKUP_USE_BUILTIN) orelse return "";
    defer c.g_object_unref(info);
    const filename = c.gtk_icon_info_get_filename(info) orelse return "";
    return alloc.dupe(u8, std.mem.span(filename)) catch "";
}

fn stripFieldCodes(alloc: std.mem.Allocator, exec: []const u8) []const u8 {
    const codes = [_][]const u8{ "%f", "%F", "%u", "%U", "%d", "%D", "%n", "%N", "%i", "%c", "%k", "%v", "%m" };
    var result = alloc.dupe(u8, exec) catch return exec;
    for (codes) |code| {
        while (std.mem.indexOf(u8, result, code)) |pos| {
            const newResult = alloc.alloc(u8, result.len - code.len) catch break;
            @memcpy(newResult[0..pos], result[0..pos]);
            @memcpy(newResult[pos..], result[pos + code.len ..]);
            alloc.free(result);
            result = newResult;
        }
    }
    return result;
}

fn scanApps(alloc: std.mem.Allocator, apps: *std.ArrayList(App), directory: []const u8) void {
    const dir = std.fs.openDirAbsolute(directory, .{ .iterate = true }) catch return;
    var it = dir.iterate();
    while (it.next() catch null) |entry| {
        if (entry.kind != .file and entry.kind != .sym_link) continue;
        if (!std.mem.endsWith(u8, entry.name, ".desktop")) continue;

        const filePath = std.fmt.allocPrint(alloc, "{s}/{s}", .{ directory, entry.name }) catch continue;

        // find existing or append
        var appIdx: ?usize = null;
        for (apps.items, 0..) |app, i| {
            if (std.mem.endsWith(u8, app.data.file, entry.name)) {
                appIdx = i;
                break;
            }
        }
        if (appIdx == null) {
            apps.append(alloc, .{ .data = .{} }) catch continue;
            appIdx = apps.items.len - 1;
        }

        var app = &apps.items[appIdx.?];
        app.data.file = filePath;

        const f = std.fs.openFileAbsolute(filePath, .{}) catch continue;
        defer f.close();
        const content = f.readToEndAlloc(alloc, 512 * 1024) catch continue;
        defer alloc.free(content);

        var noDisplay = false;
        var inAction = false;
        var lines = std.mem.splitScalar(u8, content, '\n');
        while (lines.next()) |line| {
            if (std.mem.startsWith(u8, line, "[Desktop Action")) {
                inAction = true;
            } else if (std.mem.startsWith(u8, line, "[")) {
                inAction = std.mem.startsWith(u8, line, "[Desktop Action");
            } else if (std.mem.startsWith(u8, line, "Name=")) {
                if (!inAction) app.data.label = alloc.dupe(u8, line[5..]) catch continue;
            } else if (std.mem.startsWith(u8, line, "Exec=")) {
                if (!inAction) app.data.exec = stripFieldCodes(alloc, line[5..]);
            } else if (std.mem.startsWith(u8, line, "Icon=")) {
                if (!inAction) {
                    const iconName = line[5..];
                    if (std.mem.indexOf(u8, iconName, "/") != null) {
                        app.data.icon = alloc.dupe(u8, iconName) catch continue;
                    } else {
                        app.data.icon = resolveIconPath(alloc, iconName);
                    }
                    app.data.isCircular = isIconCircular(app.data.icon);
                }
            } else if (std.mem.startsWith(u8, line, "NoDisplay=true")) {
                if (!inAction) {
                    noDisplay = true;
                    break;
                }
            }
        }

        if (noDisplay) {
            _ = apps.orderedRemove(appIdx.?);
        }
    }
}

pub fn pinnedHas(launcher: *Launcher, file: []const u8) bool {
    const basename = std.fs.path.basename(file);
    for (launcher.pinnedApps.items) |p| {
        if (std.mem.eql(u8, p, basename)) return true;
    }
    return false;
}

pub fn pinnedToggle(launcher: *Launcher, file: []const u8) void {
    const basename = std.fs.path.basename(file);
    for (launcher.pinnedApps.items, 0..) |p, i| {
        if (std.mem.eql(u8, p, basename)) {
            _ = launcher.pinnedApps.orderedRemove(i);
            savePinned(launcher);
            return;
        }
    }
    launcher.pinnedApps.insert(launcher.alloc, 0, launcher.alloc.dupe(u8, basename) catch return) catch return;
    savePinned(launcher);
}

fn savePinned(launcher: *Launcher) void {
    const f = std.fs.createFileAbsolute(launcher.configPath, .{}) catch return;
    defer f.close();
    f.writeAll("{\"pinnedApps\":[") catch return;
    for (launcher.pinnedApps.items, 0..) |p, i| {
        if (i > 0) f.writeAll(",") catch {};
        f.writeAll("\"") catch {};
        f.writeAll(p) catch {};
        f.writeAll("\"") catch {};
    }
    f.writeAll("]}") catch {};
}

fn loadPinned(launcher: *Launcher) void {
    const f = std.fs.openFileAbsolute(launcher.configPath, .{}) catch return;
    defer f.close();
    const data = f.readToEndAlloc(launcher.alloc, 1024 * 1024) catch return;
    defer launcher.alloc.free(data);
    const parsed = std.json.parseFromSlice(LauncherConfig, launcher.alloc, data, .{ .ignore_unknown_fields = true }) catch return;
    defer parsed.deinit();
    for (parsed.value.pinnedApps) |p| {
        launcher.pinnedApps.append(launcher.alloc, launcher.alloc.dupe(u8, p) catch continue) catch {};
    }
}

const MenuCtx = struct { launcher: *Launcher, appIdx: usize };

fn openContextMenu(launcher: *Launcher, appIdx: usize) void {
    if (launcher.menu) |m| c.gtk_widget_destroy(m);
    launcher.menu = c.gtk_menu_new();
    const menu = launcher.menu.?;

    const app = &launcher.apps.items[appIdx];
    if (app.element) |el| c.gtk_widget_unset_state_flags(el, c.GTK_STATE_FLAG_PRELIGHT);

    const isPinned = pinnedHas(launcher, app.data.file);
    const pinLabel = if (isPinned) "Unpin" else "Pin";
    const pinIcon = if (isPinned) "cancel" else "push_pin";

    const pinItem = makeMenuItem(pinLabel, pinIcon);
    const pinCtx = launcher.alloc.create(MenuCtx) catch unreachable;
    pinCtx.* = .{ .launcher = launcher, .appIdx = appIdx };
    _ = c.g_signal_connect_data(pinItem, "activate", @ptrCast(&onMenuActivate), pinCtx, null, 0);
    c.gtk_menu_shell_append(@ptrCast(menu), pinItem);

    const folderItem = makeMenuItem("Open folder", "folder_open");
    const folderCtx = launcher.alloc.create(MenuCtx) catch unreachable;
    folderCtx.* = .{ .launcher = launcher, .appIdx = appIdx };
    _ = c.g_signal_connect_data(folderItem, "activate", @ptrCast(&onMenuFolderActivate), folderCtx, null, 0);
    c.gtk_menu_shell_append(@ptrCast(menu), folderItem);

    c.gtk_widget_show_all(menu);
    c.gtk_menu_popup_at_pointer(@ptrCast(menu), null);
}

fn makeIcon(name: [*:0]const u8) *c.GtkWidget {
    const box = c.gtk_box_new(c.GTK_ORIENTATION_HORIZONTAL, 0).?;
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(box), "icon");
    const lbl = c.gtk_label_new(name).?;
    c.gtk_container_add(@ptrCast(box), lbl);
    _ = c.gtk_widget_show(lbl);
    return box;
}

fn makeMenuItem(label: [:0]const u8, icon: [:0]const u8) *c.GtkWidget {
    const item = c.gtk_menu_item_new().?;
    const box = c.gtk_box_new(c.GTK_ORIENTATION_HORIZONTAL, 4).?;
    if (icon.len > 0) {
        const ico = makeIcon(icon);
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(ico), "start-icon");
        c.gtk_box_pack_start(@ptrCast(box), ico, 0, 0, 0);
        _ = c.gtk_widget_show(ico);
    }
    const lbl = c.gtk_label_new(label).?;
    c.gtk_box_pack_start(@ptrCast(box), lbl, 0, 0, 0);
    _ = c.gtk_widget_show(lbl);
    _ = c.gtk_widget_show(box);
    c.gtk_container_add(@ptrCast(item), box);
    return item;
}

fn clearMenuShell(menu: *c.GtkWidget) void {
    c.shim_container_clear(menu);
}

fn onMenuActivate(_: *anyopaque, data: *MenuCtx) callconv(.c) void {
    const app = &data.launcher.apps.items[data.appIdx];
    pinnedToggle(data.launcher, app.data.file);
    updateGrid(data.launcher, true);
}

fn onMenuFolderActivate(_: *anyopaque, data: *MenuCtx) callconv(.c) void {
    const app = &data.launcher.apps.items[data.appIdx];
    const dir = std.fs.path.dirname(app.data.file) orelse return;
    var buf: [2048]u8 = undefined;
    const cmd = std.fmt.bufPrint(&buf, "xdg-open {s}", .{dir}) catch return;
    spawnCommand(cmd);
}

fn spawnCommand(command: []const u8) void {
    var buf: [2048]u8 = undefined;
    const z = std.fmt.bufPrintZ(&buf, "{s}", .{command}) catch return;
    const pid = std.posix.fork() catch return;
    if (pid == 0) {
        _ = std.posix.execvpeZ("/bin/sh", &[_:null]?[*:0]const u8{ "/bin/sh", "-c", z, null }, @ptrCast(std.os.environ.ptr)) catch {};
        std.process.exit(1);
    }
}

pub const AppClickCtx = struct { launcher: *Launcher, appIdx: usize };
pub fn updateGrid(launcher: *Launcher, sort: bool) void {
    const pinGrid = launcher.pinGrid orelse return;
    const grid = launcher.grid orelse return;
    const searchWidget = launcher.search orelse return;

    if (sort) sortApps(launcher);

    clearFlowBox(pinGrid);
    clearFlowBox(grid);

    const searchText = std.mem.span(c.gtk_entry_get_text(@ptrCast(searchWidget)));

    for (launcher.apps.items, 0..) |*app, i| {
        if (searchText.len > 0 and !searchContains(app.data.label, searchText)) continue;

        // Icon widget
        const iconBox = c.gtk_box_new(c.GTK_ORIENTATION_HORIZONTAL, 0).?;
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(iconBox), "icon");
        if (app.data.isCircular) {
            c.gtk_style_context_add_class(c.gtk_widget_get_style_context(iconBox), "circular");
        } else {
            c.gtk_style_context_add_class(c.gtk_widget_get_style_context(iconBox), "adaptive");
        }
        if (app.data.icon.len > 0) {
            c.gtk_style_context_add_class(c.gtk_widget_get_style_context(iconBox), "image");
            var cssBuf2: [1024]u8 = undefined;
            const iconCss = std.fmt.bufPrintZ(&cssBuf2, "* {{ background-image: url(\"{s}\"); }}", .{app.data.icon}) catch "";
            if (iconCss.len > 0) {
                const iconProvider = c.gtk_css_provider_new();
                _ = c.gtk_css_provider_load_from_data(iconProvider, iconCss, -1, null);
                c.gtk_style_context_add_provider(c.gtk_widget_get_style_context(iconBox), @ptrCast(iconProvider), c.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            }
        }
        c.gtk_widget_set_halign(iconBox, c.GTK_ALIGN_CENTER);

        // Name label
        var lblBuf: [256]u8 = undefined;
        const labelZ = std.fmt.bufPrintZ(&lblBuf, "{s}", .{app.data.label}) catch "";
        const nameLabel = c.gtk_label_new(labelZ).?;
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(nameLabel), "name");
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(nameLabel), "text-sm");
        c.gtk_label_set_ellipsize(@ptrCast(nameLabel), c.PANGO_ELLIPSIZE_END);

        // VBox
        const vbox = c.gtk_box_new(c.GTK_ORIENTATION_VERTICAL, 0).?;
        c.gtk_container_add(@ptrCast(vbox), iconBox);
        c.gtk_container_add(@ptrCast(vbox), nameLabel);
        _ = c.gtk_widget_show(iconBox);
        _ = c.gtk_widget_show(nameLabel);

        // EventBox for hover/click
        const evBox = c.gtk_event_box_new().?;
        c.gtk_container_add(@ptrCast(evBox), vbox);
        _ = c.gtk_widget_show(vbox);

        // FlowBoxChild
        const fbc = c.gtk_flow_box_child_new().?;
        c.gtk_container_add(@ptrCast(fbc), evBox);
        c.gtk_style_context_add_class(c.gtk_widget_get_style_context(fbc), "app");
        app.element = fbc;

        // Connect signals — pointer down for context menu
        const clickCtx = launcher.alloc.create(AppClickCtx) catch continue;
        clickCtx.* = .{ .launcher = launcher, .appIdx = i };
        c.gtk_widget_add_events(evBox, c.GDK_BUTTON_PRESS_MASK | c.GDK_ENTER_NOTIFY_MASK | c.GDK_LEAVE_NOTIFY_MASK);
        _ = c.g_signal_connect_data(evBox, "button-press-event", @ptrCast(&onAppPointerDown), clickCtx, null, 0);
        _ = c.g_signal_connect_data(evBox, "enter-notify-event", @ptrCast(&onAppHoverIn), fbc, null, 0);
        _ = c.g_signal_connect_data(evBox, "leave-notify-event", @ptrCast(&onAppHoverOut), fbc, null, 0);

        dragDrop.setupSource(launcher, evBox, i);

        const targetGrid = if (pinnedHas(launcher, app.data.file)) pinGrid else grid;
        c.gtk_container_add(@ptrCast(targetGrid), fbc);
        _ = c.gtk_widget_show_all(fbc);
    }

    c.gtk_widget_show_all(pinGrid);
    c.gtk_widget_show_all(grid);

    const hasPinned = c.gtk_flow_box_get_child_at_index(@ptrCast(pinGrid), 0) != null;
    c.gtk_widget_set_visible(pinGrid, if (hasPinned) 1 else 0);

    if (launcher.searchPlaceholder) |ph| {
        const hasAny = hasPinned or c.gtk_flow_box_get_child_at_index(@ptrCast(grid), 0) != null;
        c.gtk_widget_set_visible(ph, if (hasAny) 0 else 1);
    }
}

fn clearFlowBox(fb: *c.GtkWidget) void {
    c.shim_container_clear(fb);
}

fn onAppHoverIn(_: *c.GtkWidget, _: ?*anyopaque, fbc: *c.GtkWidget) callconv(.{ .x86_64_sysv = .{} }) c.gboolean {
    c.gtk_widget_set_state_flags(fbc, c.GTK_STATE_FLAG_PRELIGHT, 0);
    return @intFromBool(false);
}

fn onAppHoverOut(_: *c.GtkWidget, _: ?*anyopaque, fbc: *c.GtkWidget) callconv(.{ .x86_64_sysv = .{} }) c.gboolean {
    c.gtk_widget_unset_state_flags(fbc, c.GTK_STATE_FLAG_PRELIGHT);
    return @intFromBool(false);
}

fn onAppPointerDown(_: *c.GtkWidget, event: ?*anyopaque, ctx: *AppClickCtx) callconv(.{ .x86_64_sysv = .{} }) c.gboolean {
    if (c.launcher_event_is_secondary_button(event) != 0) {
        openContextMenu(ctx.launcher, ctx.appIdx);
        return @intFromBool(true);
    }
    return @intFromBool(false);
}

fn searchContains(text: []const u8, query: []const u8) bool {
    var textLow: [512]u8 = undefined;
    var queryLow: [512]u8 = undefined;
    const tl = std.ascii.lowerString(textLow[0..@min(text.len, 511)], text[0..@min(text.len, 511)]);
    const ql = std.ascii.lowerString(queryLow[0..@min(query.len, 511)], query[0..@min(query.len, 511)]);
    return std.mem.indexOf(u8, tl, ql) != null;
}

fn sortApps(launcher: *Launcher) void {
    const pinned = launcher.pinnedApps.items;
    std.sort.insertion(App, launcher.apps.items, pinned, struct {
        fn lt(pins: [][]const u8, a: App, b: App) bool {
            const ia = pinnedIndex(pins, a.data.file);
            const ib = pinnedIndex(pins, b.data.file);
            if (ia != ib) return ia < ib;
            return std.ascii.lessThanIgnoreCase(a.data.label, b.data.label);
        }
    }.lt);
}

fn pinnedIndex(pinned: [][]const u8, file: []const u8) usize {
    const basename = std.fs.path.basename(file);
    for (pinned, 0..) |p, i| if (std.mem.eql(u8, p, basename)) return i;
    return pinned.len + 1; // not pinned → sort last
}

fn onChildActivated(_: *c.GtkFlowBox, child: *c.GtkFlowBoxChild, data: ?*anyopaque) callconv(.{ .x86_64_sysv = .{} }) void {
    const launcher: *Launcher = @ptrCast(@alignCast(data));
    for (launcher.apps.items) |app| {
        if (app.element == @as(?*c.GtkWidget, @ptrCast(child))) {
            var buf: [2048]u8 = undefined;
            const cmd = std.fmt.bufPrint(&buf, "{s}", .{app.data.exec}) catch return;
            spawnCommand(cmd);
            return;
        }
    }
}

fn onSearchChanged(_: *c.GtkWidget, data: ?*anyopaque) callconv(.{ .x86_64_sysv = .{} }) void {
    const launcher: *Launcher = @ptrCast(@alignCast(data));
    updateGrid(launcher, false);
}

fn onSearchSubmit(_: *c.GtkWidget, data: ?*anyopaque) callconv(.{ .x86_64_sysv = .{} }) void {
    const launcher: *Launcher = @ptrCast(@alignCast(data));
    if (launcher.pinGrid) |pg| {
        if (c.gtk_flow_box_get_child_at_index(@ptrCast(pg), 0)) |child| {
            _ = c.gtk_widget_activate(@ptrCast(child));
            return;
        }
    }
    if (launcher.grid) |g| {
        if (c.gtk_flow_box_get_child_at_index(@ptrCast(g), 0)) |child| {
            _ = c.gtk_widget_activate(@ptrCast(child));
        }
    }
}

fn onKeyDown(_: *c.GtkWidget, event: ?*anyopaque, data: ?*anyopaque) callconv(.{ .x86_64_sysv = .{} }) c.gboolean {
    const launcher: *Launcher = @ptrCast(@alignCast(data));
    const keyval = c.launcher_event_keyval(event);
    if (keyval == c.GDK_KEY_Escape) {
        unloadSelf(launcher);
        return @intFromBool(true);
    }
    return @intFromBool(false);
}

fn loadExtensionCss(launcher: *Launcher) void {
    var buf: [4096]u8 = undefined;
    const path = std.fmt.bufPrintZ(&buf, "{s}/default.css", .{launcher.extDir}) catch return;
    const provider = c.gtk_css_provider_new();
    _ = c.gtk_css_provider_load_from_path(provider, path, null);
    c.gtk_style_context_add_provider_for_screen(
        c.gdk_screen_get_default(),
        @ptrCast(provider),
        c.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION,
    );
}

fn createWindow(launcher: *Launcher) void {
    const win = c.gtk_window_new(c.GTK_WINDOW_TOPLEVEL).?;
    c.gtk_layer_init_for_window(@ptrCast(win));
    c.gtk_layer_set_layer(@ptrCast(win), c.GTK_LAYER_SHELL_LAYER_TOP);
    // on_demand required — exclusive mode blocks popup menu click events (activate signal never fires)
    c.gtk_layer_set_keyboard_mode(@ptrCast(win), .on_demand);
    c.gtk_layer_set_namespace(@ptrCast(win), "launcher");
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(win), "launcher");
    c.gtk_widget_set_size_request(win, 440, 540);

    _ = c.g_signal_connect_data(win, "key-press-event", @ptrCast(&onKeyDown), launcher, null, 0);

    loadExtensionCss(launcher);

    const body = c.gtk_box_new(c.GTK_ORIENTATION_VERTICAL, 0).?;
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(body), "body");

    const searchBox = c.gtk_box_new(c.GTK_ORIENTATION_HORIZONTAL, 0).?;
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(searchBox), "search");

    const searchIcon = makeIcon("search");
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(searchIcon), "start-icon");
    c.gtk_box_pack_start(@ptrCast(searchBox), searchIcon, 0, 0, 0);
    _ = c.gtk_widget_show(searchIcon);

    const searchEntry = c.gtk_entry_new().?;
    c.gtk_widget_set_hexpand(searchEntry, 1);
    _ = c.g_signal_connect_data(searchEntry, "changed", @ptrCast(&onSearchChanged), launcher, null, 0);
    _ = c.g_signal_connect_data(searchEntry, "activate", @ptrCast(&onSearchSubmit), launcher, null, 0);
    c.gtk_box_pack_start(@ptrCast(searchBox), searchEntry, 1, 1, 0);
    _ = c.gtk_widget_show(searchEntry);
    launcher.search = searchEntry;

    c.gtk_box_pack_start(@ptrCast(body), searchBox, 0, 0, 0);
    _ = c.gtk_widget_show(searchBox);

    const container = c.gtk_box_new(c.GTK_ORIENTATION_VERTICAL, 0).?;

    const pinGridW = c.gtk_flow_box_new().?;
    c.gtk_flow_box_set_homogeneous(@ptrCast(pinGridW), 1);
    c.gtk_flow_box_set_min_children_per_line(@ptrCast(pinGridW), 3);
    c.gtk_flow_box_set_max_children_per_line(@ptrCast(pinGridW), 3);
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(pinGridW), "grid");
    _ = c.g_signal_connect_data(pinGridW, "child-activated", @ptrCast(&onChildActivated), launcher, null, 0);
    launcher.pinGrid = pinGridW;
    c.gtk_box_pack_start(@ptrCast(container), pinGridW, 0, 0, 0);
    _ = c.gtk_widget_show(pinGridW);

    const gridW = c.gtk_flow_box_new().?;
    c.gtk_flow_box_set_homogeneous(@ptrCast(gridW), 1);
    c.gtk_flow_box_set_min_children_per_line(@ptrCast(gridW), 3);
    c.gtk_flow_box_set_max_children_per_line(@ptrCast(gridW), 3);
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(gridW), "grid");
    _ = c.g_signal_connect_data(gridW, "child-activated", @ptrCast(&onChildActivated), launcher, null, 0);
    launcher.grid = gridW;
    c.gtk_box_pack_start(@ptrCast(container), gridW, 0, 0, 0);
    _ = c.gtk_widget_show(gridW);

    const placeholder = c.gtk_box_new(c.GTK_ORIENTATION_VERTICAL, 24).?;
    c.gtk_style_context_add_class(c.gtk_widget_get_style_context(placeholder), "placeholder");
    const phIcon = makeIcon("apps");
    c.gtk_widget_set_halign(phIcon, c.GTK_ALIGN_CENTER);
    const phLabel = c.gtk_label_new("No results").?;
    c.gtk_box_pack_start(@ptrCast(placeholder), phIcon, 0, 0, 0);
    c.gtk_box_pack_start(@ptrCast(placeholder), phLabel, 0, 0, 0);
    _ = c.gtk_widget_show(phIcon);
    _ = c.gtk_widget_show(phLabel);
    launcher.searchPlaceholder = placeholder;
    c.gtk_box_pack_start(@ptrCast(container), placeholder, 0, 0, 0);
    _ = c.gtk_widget_show(placeholder);

    const scroll = c.gtk_scrolled_window_new(null, null).?;
    c.gtk_scrolled_window_set_policy(@ptrCast(scroll), c.GTK_POLICY_AUTOMATIC, c.GTK_POLICY_AUTOMATIC);
    c.gtk_widget_set_vexpand(scroll, 1);
    c.gtk_container_add(@ptrCast(scroll), container);
    _ = c.gtk_widget_show(container);
    c.gtk_box_pack_start(@ptrCast(body), scroll, 1, 1, 0);
    _ = c.gtk_widget_show(scroll);

    c.gtk_container_add(@ptrCast(win), body);
    _ = c.gtk_widget_show(body);
    c.gtk_widget_show_all(win);

    launcher.window = win;

    updateGrid(launcher, true);
    dragDrop.setupDropTargets(launcher);
    c.gtk_widget_grab_focus(searchEntry);
}

pub fn pinnedReorder(launcher: *Launcher, filename: []const u8, newIdx: usize) void {
    const basename = std.fs.path.basename(filename);
    for (launcher.pinnedApps.items, 0..) |p, i| {
        if (std.mem.eql(u8, p, basename)) {
            if (i == newIdx) return;
            const item = launcher.pinnedApps.orderedRemove(i);
            const clampedIdx = @min(newIdx, launcher.pinnedApps.items.len);
            launcher.pinnedApps.insert(launcher.alloc, clampedIdx, item) catch {};
            savePinned(launcher);
            return;
        }
    }
}

pub fn pinnedInsertAt(launcher: *Launcher, filename: []const u8, idx: usize) void {
    const basename = std.fs.path.basename(filename);
    for (launcher.pinnedApps.items) |p| if (std.mem.eql(u8, p, basename)) return;
    const clampedIdx = @min(idx, launcher.pinnedApps.items.len);
    launcher.pinnedApps.insert(launcher.alloc, clampedIdx, launcher.alloc.dupe(u8, basename) catch return) catch {};
    savePinned(launcher);
}

fn unloadSelf(launcher: *Launcher) void {
    if (launcher.window) |w| {
        c.gtk_widget_destroy(w);
        launcher.window = null;
        launcher.pinGrid = null;
        launcher.grid = null;
        launcher.search = null;
        launcher.searchPlaceholder = null;
        launcher.menu = null;
        for (launcher.apps.items) |*app| app.element = null;
    }
}

fn onRequest(extPtr: *Extension, command: [*:0]const u8) callconv(.{ .x86_64_sysv = .{} }) CResponse {
    const launcher: *Launcher = @fieldParentPtr("ext", extPtr);
    const cmd = std.mem.span(command);

    if (std.mem.eql(u8, cmd, "toggle")) {
        if (launcher.window != null) {
            unloadSelf(launcher);
            return .{ .content = "Launcher closed", .status = 0 };
        }
        createWindow(launcher);
        return .{ .content = "Launcher opened", .status = 0 };
    }
    return .{ .content = "Unknown command", .status = 1 };
}

fn deinit(extPtr: *Extension) callconv(.{ .x86_64_sysv = .{} }) void {
    const launcher: *Launcher = @fieldParentPtr("ext", extPtr);
    unloadSelf(launcher);
    launcher.apps.deinit(launcher.alloc);
    launcher.pinnedApps.deinit(launcher.alloc);
    std.heap.c_allocator.destroy(launcher);
}

export fn createExtension() *Extension {
    const alloc = std.heap.c_allocator;
    const launcher = alloc.create(Launcher) catch @panic("OOM");

    const home = std.posix.getenv("HOME") orelse "/root";
    const configDir = std.fmt.allocPrint(alloc, "{s}/.config/system-ui", .{home}) catch @panic("OOM");
    const cacheDir = std.fmt.allocPrint(alloc, "{s}/.cache/system-ui", .{home}) catch @panic("OOM");
    const extDir = blk: {
        var info: c.Dl_info = undefined;
        if (c.dladdr(@ptrCast(&createExtension), &info) != 0) {
            if (info.dli_fname) |fname| {
                const path = std.mem.span(fname);
                if (std.fs.path.dirname(path)) |dir|
                    break :blk alloc.dupe(u8, dir) catch "/usr/share/system-ui/extensions/launcher";
            }
        }
        break :blk alloc.dupe(u8, "/usr/share/system-ui/extensions/launcher") catch @panic("OOM");
    };

    launcher.* = .{
        .ext = .{
            .handle = null,
            .onRequestFn = onRequest,
            .deinitFn = deinit,
        },
        .alloc = alloc,
        .apps = std.ArrayList(App){},
        .pinnedApps = std.ArrayList([]const u8){},
        .configPath = std.fmt.allocPrint(alloc, "{s}/launcher.json", .{configDir}) catch @panic("OOM"),
        .cachePath = std.fmt.allocPrint(alloc, "{s}/launcher.json", .{cacheDir}) catch @panic("OOM"),
        .userCssPath = std.fmt.allocPrint(alloc, "{s}/system-ui.css", .{configDir}) catch @panic("OOM"),
        .extDir = alloc.dupe(u8, extDir) catch @panic("OOM"),
    };

    loadPinned(launcher);

    var cacheLoaded = false;
    if (std.fs.openFileAbsolute(launcher.cachePath, .{}) catch null) |f| {
        defer f.close();
        const data = f.readToEndAlloc(alloc, 10 * 1024 * 1024) catch null;
        if (data) |d| {
            defer alloc.free(d);
            if (std.json.parseFromSlice(AppCache, alloc, d, .{ .ignore_unknown_fields = true }) catch null) |parsed| {
                defer parsed.deinit();
                for (parsed.value.apps) |app| {
                    launcher.apps.append(alloc, .{ .data = .{
                        .file = alloc.dupe(u8, app.file) catch continue,
                        .label = alloc.dupe(u8, app.label) catch continue,
                        .exec = alloc.dupe(u8, app.exec) catch continue,
                        .icon = alloc.dupe(u8, app.icon) catch continue,
                        .isCircular = app.isCircular,
                    } }) catch {};
                }
                cacheLoaded = parsed.value.apps.len > 0;
            }
        }
    }

    const appsDir = "/usr/share/applications";
    const userAppsDir = std.fmt.allocPrint(alloc, "{s}/.local/share/applications", .{home}) catch "";

    var needScan = !cacheLoaded;
    if (!needScan) {
        // simple heuristic: always re-scan on first load if no cache timestamp stored
        needScan = true; // scan every time for correctness, cache just provides fast initial load
    }

    if (needScan) {
        launcher.apps.clearRetainingCapacity();
        scanApps(alloc, &launcher.apps, appsDir);
        if (userAppsDir.len > 0) scanApps(alloc, &launcher.apps, userAppsDir);

        // Save cache
        _ = std.fs.makeDirAbsolute(cacheDir) catch {};
        if (std.fs.createFileAbsolute(launcher.cachePath, .{}) catch null) |cf| {
            defer cf.close();
            cf.writeAll("{\"apps\":[") catch {};
            for (launcher.apps.items, 0..) |app, ai| {
                if (ai > 0) cf.writeAll(",") catch {};
                var jbuf: [4096]u8 = undefined;
                const jline = std.fmt.bufPrint(
                    &jbuf,
                    "{{\"file\":\"{s}\",\"label\":\"{s}\",\"exec\":\"{s}\",\"icon\":\"{s}\",\"isCircular\":{s}}}",
                    .{ app.data.file, app.data.label, app.data.exec, app.data.icon, if (app.data.isCircular) "true" else "false" },
                ) catch continue;
                cf.writeAll(jline) catch {};
            }
            cf.writeAll("]}") catch {};
        }
    }

    return &launcher.ext;
}
