const std = @import("std");
const c = @import("gtk.zig");

// ── Element (base) ─────────────────────────────────────────────────────────

pub const Element = struct {
    widget: *c.GtkWidget,
    children: std.ArrayList(*Element),
    alloc: std.mem.Allocator,

    pub fn initWith(alloc: std.mem.Allocator, widget: *c.GtkWidget) Element {
        return .{ .widget = widget, .children = std.ArrayList(*Element).init(alloc), .alloc = alloc };
    }

    pub fn deinit(self: *Element) void {
        for (self.children.items) |child| {
            child.deinit();
            self.alloc.destroy(child);
        }
        self.children.deinit();
        c.gtk_widget_destroy(self.widget);
    }

    pub fn add(self: *Element, child: *Element) *Element {
        c.gtk_container_add(@ptrCast(self.widget), child.widget);
        _ = c.gtk_widget_show(child.widget);
        self.children.append(child) catch {};
        return self;
    }

    pub fn visible(self: *Element, value: bool) *Element {
        c.gtk_widget_set_visible(self.widget, if (value) 1 else 0);
        return self;
    }

    pub fn show(self: *Element) *Element {
        c.gtk_widget_show(self.widget);
        return self;
    }

    pub fn addClass(self: *Element, classNames: []const u8) *Element {
        const ctx = c.gtk_widget_get_style_context(self.widget);
        var it = std.mem.tokenizeScalar(u8, classNames, ' ');
        var buf: [128]u8 = undefined;
        while (it.next()) |name| {
            const z = std.fmt.bufPrintZ(&buf, "{s}", .{name}) catch continue;
            c.gtk_style_context_add_class(ctx, z);
        }
        return self;
    }

    pub fn removeClass(self: *Element, className: []const u8) *Element {
        const ctx = c.gtk_widget_get_style_context(self.widget);
        var buf: [128]u8 = undefined;
        const z = std.fmt.bufPrintZ(&buf, "{s}", .{className}) catch return self;
        c.gtk_style_context_remove_class(ctx, z);
        return self;
    }

    pub fn size(self: *Element, w: i16, h: i16) *Element {
        c.gtk_widget_set_size_request(self.widget, w, h);
        return self;
    }

    pub fn focus(self: *Element) *Element {
        c.gtk_widget_grab_focus(self.widget);
        return self;
    }

    pub fn addState(self: *Element, flag: c.GtkStateFlags) *Element {
        const flags = c.gtk_widget_get_state_flags(self.widget);
        if (flags & @intFromEnum(flag) == 0)
            c.gtk_widget_set_state_flags(self.widget, flag, 0);
        return self;
    }

    pub fn removeState(self: *Element, flag: c.GtkStateFlags) *Element {
        const flags = c.gtk_widget_get_state_flags(self.widget);
        if (flags & @intFromEnum(flag) != 0)
            c.gtk_widget_unset_state_flags(self.widget, flag);
        return self;
    }
};

// ── Box ────────────────────────────────────────────────────────────────────

pub const Box = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator, orientation: c.GtkOrientation) Box {
        const w = c.gtk_box_new(orientation, 0);
        c.gtk_box_set_homogeneous(@ptrCast(w), 0);
        return .{ .base = Element.initWith(alloc, w.?) };
    }

    pub fn gap(self: *Box, value: u16) *Box {
        c.gtk_box_set_spacing(@ptrCast(self.base.widget), @intCast(value));
        return self;
    }

    pub fn spaceEvenly(self: *Box, value: bool) *Box {
        c.gtk_box_set_homogeneous(@ptrCast(self.base.widget), if (value) 1 else 0);
        return self;
    }

    pub fn prependChild(self: *Box, child: *Element) *Box {
        c.gtk_box_pack_start(@ptrCast(self.base.widget), child.widget, 1, 1, 0);
        _ = c.gtk_widget_show(child.widget);
        self.base.children.append(child) catch {};
        return self;
    }
};

// ── Label ──────────────────────────────────────────────────────────────────

pub const Label = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator, text: []const u8) Label {
        var buf: [512]u8 = undefined;
        const z = std.fmt.bufPrintZ(&buf, "{s}", .{text}) catch buf[0..1 :0];
        return .{ .base = Element.initWith(alloc, c.gtk_label_new(z).?) };
    }

    pub fn set(self: *Label, text: []const u8) *Label {
        var buf: [512]u8 = undefined;
        const z = std.fmt.bufPrintZ(&buf, "{s}", .{text}) catch return self;
        c.gtk_label_set_text(@ptrCast(self.base.widget), z);
        return self;
    }
};

// ── Icon ───────────────────────────────────────────────────────────────────

pub const Icon = struct {
    box: Box,
    label: ?*Label = null,

    pub fn init(alloc: std.mem.Allocator) Icon {
        var icon = Icon{ .box = Box.init(alloc, c.GTK_ORIENTATION_HORIZONTAL) };
        _ = icon.box.base.addClass("icon");
        return icon;
    }

    pub fn set(self: *Icon, name: []const u8) *Icon {
        if (self.label == null) {
            const lbl = self.box.base.alloc.create(Label) catch return self;
            lbl.* = Label.init(self.box.base.alloc, "");
            _ = self.box.base.add(&lbl.base);
            self.label = lbl;
        }
        _ = self.label.?.set(name);
        return self;
    }

    pub fn setImage(self: *Icon, path: []const u8) *Icon {
        const ctx = c.gtk_widget_get_style_context(self.box.base.widget);
        if (c.gtk_style_context_has_class(ctx, "image") == 0)
            _ = self.box.base.addClass("image");
        var cssBuf: [1024]u8 = undefined;
        const css = std.fmt.bufPrintZ(&cssBuf, "* {{ background-image: url(\"{s}\"); }}", .{path}) catch return self;
        applyCssToWidget(self.box.base.widget, css);
        return self;
    }
};

fn applyCssToWidget(widget: *c.GtkWidget, css: [:0]const u8) void {
    const provider = c.gtk_css_provider_new();
    defer c.g_object_unref(provider);
    _ = c.gtk_css_provider_load_from_data(provider, css, -1, null);
    const ctx = c.gtk_widget_get_style_context(widget);
    c.gtk_style_context_add_provider(ctx, @ptrCast(provider), c.GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// ── Input ──────────────────────────────────────────────────────────────────

pub const Input = struct {
    base: Element,
    changeCallback: ?*const fn (data: ?*anyopaque) void = null,
    submitCallback: ?*const fn (data: ?*anyopaque) void = null,
    userData: ?*anyopaque = null,

    pub fn init(alloc: std.mem.Allocator) Input {
        const w = c.gtk_entry_new().?;
        c.gtk_widget_set_hexpand(w, 1);
        return .{ .base = Element.initWith(alloc, w) };
    }

    pub fn getValue(self: *Input) []const u8 {
        return std.mem.span(c.gtk_entry_get_text(@ptrCast(self.base.widget)));
    }

    pub fn setValue(self: *Input, val: []const u8) *Input {
        var buf: [512]u8 = undefined;
        const z = std.fmt.bufPrintZ(&buf, "{s}", .{val}) catch return self;
        c.gtk_entry_set_text(@ptrCast(self.base.widget), z);
        return self;
    }

    pub fn onChangeFn(self: *Input, cb: *const fn (data: ?*anyopaque) void, data: ?*anyopaque) *Input {
        self.changeCallback = cb;
        self.userData = data;
        _ = c.g_signal_connect_data(self.base.widget, "changed", @ptrCast(&inputChangedCb), data, null, 0);
        return self;
    }

    pub fn onSubmitFn(self: *Input, cb: *const fn (data: ?*anyopaque) void, data: ?*anyopaque) *Input {
        self.submitCallback = cb;
        _ = c.g_signal_connect_data(self.base.widget, "activate", @ptrCast(&inputActivateCb), data, null, 0);
        return self;
    }
};

fn inputChangedCb(_: *c.GtkWidget, data: ?*anyopaque) callconv(.C) void {
    _ = data;
}
fn inputActivateCb(_: *c.GtkWidget, data: ?*anyopaque) callconv(.C) void {
    _ = data;
}

// ── FlowBoxChild / FlowBox ─────────────────────────────────────────────────

pub const FlowBoxChild = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator) FlowBoxChild {
        return .{ .base = Element.initWith(alloc, c.gtk_flow_box_child_new().?) };
    }
};

pub const FlowBox = struct {
    base: Element,
    childClickCb: ?*const fn (child: *c.GtkFlowBoxChild, data: ?*anyopaque) void = null,
    cbData: ?*anyopaque = null,

    pub fn init(alloc: std.mem.Allocator) FlowBox {
        const w = c.gtk_flow_box_new().?;
        c.gtk_flow_box_set_homogeneous(@ptrCast(w), 1);
        return .{ .base = Element.initWith(alloc, w) };
    }

    pub fn columns(self: *FlowBox, n: u8) *FlowBox {
        c.gtk_flow_box_set_min_children_per_line(@ptrCast(self.base.widget), n);
        c.gtk_flow_box_set_max_children_per_line(@ptrCast(self.base.widget), n);
        return self;
    }

    pub fn onChildClickFn(self: *FlowBox, cb: *const fn (*c.GtkFlowBoxChild, ?*anyopaque) void, data: ?*anyopaque) *FlowBox {
        self.childClickCb = cb;
        self.cbData = data;
        _ = c.g_signal_connect_data(self.base.widget, "child-activated", @ptrCast(&flowChildActivated), data, null, 0);
        return self;
    }

    pub fn addChild(self: *FlowBox, child: *Element) *FlowBoxChild {
        const alloc = self.base.alloc;
        const fbc = alloc.create(FlowBoxChild) catch unreachable;
        fbc.* = FlowBoxChild.init(alloc);
        _ = fbc.base.add(child);
        c.gtk_container_add(@ptrCast(self.base.widget), fbc.base.widget);
        _ = c.gtk_widget_show(fbc.base.widget);
        self.base.children.append(&fbc.base) catch {};
        return fbc;
    }
};

fn flowChildActivated(_: *c.GtkFlowBox, child: *c.GtkFlowBoxChild, data: ?*anyopaque) callconv(.C) void {
    _ = data;
    _ = child;
}

// ── ScrolledWindow ─────────────────────────────────────────────────────────

pub const ScrolledWindow = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator) ScrolledWindow {
        const w = c.gtk_scrolled_window_new(null, null).?;
        c.gtk_scrolled_window_set_policy(@ptrCast(w), c.GTK_POLICY_AUTOMATIC, c.GTK_POLICY_AUTOMATIC);
        c.gtk_widget_set_vexpand(w, 1);
        return .{ .base = Element.initWith(alloc, w) };
    }
};

// ── EventBox ───────────────────────────────────────────────────────────────

pub const EventBox = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator) EventBox {
        return .{ .base = Element.initWith(alloc, c.gtk_event_box_new().?) };
    }

    pub fn onPointerDown(self: *EventBox, cb: c.GCallback, data: ?*anyopaque) *EventBox {
        c.gtk_widget_add_events(self.base.widget, c.GDK_BUTTON_PRESS_MASK);
        _ = c.g_signal_connect_data(self.base.widget, "button-press-event", cb, data, null, 0);
        return self;
    }

    pub fn onHover(self: *EventBox, cb: c.GCallback, data: ?*anyopaque) *EventBox {
        c.gtk_widget_add_events(self.base.widget, c.GDK_ENTER_NOTIFY_MASK);
        _ = c.g_signal_connect_data(self.base.widget, "enter-notify-event", cb, data, null, 0);
        return self;
    }

    pub fn onHoverOut(self: *EventBox, cb: c.GCallback, data: ?*anyopaque) *EventBox {
        c.gtk_widget_add_events(self.base.widget, c.GDK_LEAVE_NOTIFY_MASK);
        _ = c.g_signal_connect_data(self.base.widget, "leave-notify-event", cb, data, null, 0);
        return self;
    }
};

// ── Window ─────────────────────────────────────────────────────────────────

pub const KeyboardMode = enum(c_int) {
    None = 0,
    Exclusive = 1,
    OnDemand = 2,
};

pub const Window = struct {
    base: EventBox,

    pub fn init(alloc: std.mem.Allocator, keyboardMode: KeyboardMode) Window {
        const w = c.gtk_window_new(c.GTK_WINDOW_TOPLEVEL).?;
        c.gtk_layer_init_for_window(@ptrCast(w));
        c.gtk_layer_set_layer(@ptrCast(w), c.GTK_LAYER_SHELL_LAYER_TOP);
        c.gtk_layer_set_keyboard_mode(@ptrCast(w), @intFromEnum(keyboardMode));
        return .{ .base = .{ .base = Element.initWith(alloc, w) } };
    }

    pub fn setNamespace(self: *Window, name: [:0]const u8) void {
        c.gtk_layer_set_namespace(@ptrCast(self.base.base.widget), name);
    }

    pub fn onKeyDown(self: *Window, cb: c.GCallback, data: ?*anyopaque) void {
        _ = c.g_signal_connect_data(self.base.base.widget, "key-press-event", cb, data, null, 0);
    }
};

// ── MenuItem / Menu ────────────────────────────────────────────────────────

pub const MenuItem = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator, label: []const u8, icon: []const u8) MenuItem {
        const w = c.gtk_menu_item_new().?;
        var item = MenuItem{ .base = Element.initWith(alloc, w) };

        const box = alloc.create(Box) catch unreachable;
        box.* = Box.init(alloc, c.GTK_ORIENTATION_HORIZONTAL);

        if (icon.len > 0) {
            const ico = alloc.create(Icon) catch unreachable;
            ico.* = Icon.init(alloc);
            _ = ico.set(icon);
            _ = ico.box.base.addClass("start-icon");
            _ = box.base.add(&ico.box.base);
        }

        const lbl = alloc.create(Label) catch unreachable;
        lbl.* = Label.init(alloc, label);
        _ = box.base.add(&lbl.base);
        _ = item.base.add(&box.base);
        return item;
    }

    pub fn onClick(self: *MenuItem, cb: c.GCallback, data: ?*anyopaque) *MenuItem {
        _ = c.g_signal_connect_data(self.base.widget, "activate", cb, data, null, 0);
        return self;
    }
};

pub const MenuSeparator = struct {
    base: Element,
    pub fn init(alloc: std.mem.Allocator) MenuSeparator {
        return .{ .base = Element.initWith(alloc, c.gtk_separator_menu_item_new().?) };
    }
};

pub const Menu = struct {
    base: Element,

    pub fn init(alloc: std.mem.Allocator) Menu {
        return .{ .base = Element.initWith(alloc, c.gtk_menu_new().?) };
    }

    pub fn addItem(self: *Menu, child: *Element) void {
        c.gtk_menu_shell_append(@ptrCast(self.base.widget), child.widget);
        _ = c.gtk_widget_show(child.widget);
        self.base.children.append(child) catch {};
    }

    pub fn clearItems(self: *Menu) void {
        for (self.base.children.items) |child| {
            c.gtk_container_remove(@ptrCast(self.base.widget), child.widget);
        }
        self.base.children.clearRetainingCapacity();
    }

    pub fn popup(self: *Menu) void {
        c.gtk_menu_popup_at_pointer(@ptrCast(self.base.widget), null);
    }

    pub fn onHide(self: *Menu, cb: c.GCallback, data: ?*anyopaque) void {
        _ = c.g_signal_connect_data(self.base.widget, "hide", cb, data, null, 0);
    }
};
