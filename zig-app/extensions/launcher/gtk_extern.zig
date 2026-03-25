// Manual extern declarations for GTK3 / GLib functions needed by launcher.
// This avoids the Zig cImport limitation with GdkEvent union containing opaque types.

pub const gboolean = c_int;
pub const gint = c_int;
pub const guint = c_uint;
pub const guint8 = u8;
pub const guint32 = u32;
pub const gint8 = i8;
pub const gchar = u8;
pub const gdouble = f64;
pub const gfloat = f32;
pub const gpointer = ?*anyopaque;
pub const gconstpointer = ?*const anyopaque;
pub const gulong = c_ulong;
pub const gsize = usize;

pub const GtkWidget = opaque {};
pub const GtkWindow = opaque {};
pub const GtkBox = opaque {};
pub const GtkLabel = opaque {};
pub const GtkEntry = opaque {};
pub const GtkFlowBox = opaque {};
pub const GtkFlowBoxChild = opaque {};
pub const GtkContainer = opaque {};
pub const GtkMenuShell = opaque {};
pub const GtkMenuItem = opaque {};
pub const GtkMenu = opaque {};
pub const GtkScrolledWindow = opaque {};
pub const GtkEventBox = opaque {};
pub const GtkStyleContext = opaque {};
pub const GtkCssProvider = opaque {};
pub const GdkScreen = opaque {};
pub const GdkDragContext = opaque {};
pub const GtkSelectionData = opaque {};
pub const GtkIconTheme = opaque {};
pub const GtkIconInfo = opaque {};
pub const GList = opaque {};
pub const GError = opaque {};
pub const GdkAtom = opaque {};
pub const GObject = opaque {};
pub const GdkPixbuf = opaque {};

pub const GtkOrientation = enum(c_int) {
    horizontal = 0,
    vertical = 1,
};
pub const GTK_ORIENTATION_HORIZONTAL: GtkOrientation = .horizontal;
pub const GTK_ORIENTATION_VERTICAL: GtkOrientation = .vertical;

pub const GtkAlign = enum(c_int) {
    fill = 0,
    start = 1,
    end = 2,
    center = 3,
    baseline = 4,
};
pub const GTK_ALIGN_CENTER: GtkAlign = .center;
pub const GTK_ALIGN_START: GtkAlign = .start;
pub const GTK_ALIGN_FILL: GtkAlign = .fill;

pub const GtkWindowType = enum(c_int) { toplevel = 0, popup = 1 };
pub const GTK_WINDOW_TOPLEVEL: GtkWindowType = .toplevel;

pub const GtkLayerShellLayer = enum(c_int) { background = 0, bottom = 1, top = 2, overlay = 3 };
pub const GTK_LAYER_SHELL_LAYER_TOP: GtkLayerShellLayer = .top;

pub const GtkLayerShellKeyboardMode = enum(c_int) { none = 0, exclusive = 1, on_demand = 2 };
pub const GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE: GtkLayerShellKeyboardMode = .exclusive;

pub const GtkPolicyType = enum(c_int) { always = 0, automatic = 1, never = 2, external = 3 };
pub const GTK_POLICY_AUTOMATIC: GtkPolicyType = .automatic;

pub const GtkStateFlags = enum(c_uint) { normal = 0, active = 1, prelight = 2, selected = 4 };
pub const GTK_STATE_FLAG_ACTIVE: GtkStateFlags = .active;
pub const GTK_STATE_FLAG_PRELIGHT: GtkStateFlags = .prelight;

pub const GtkIconLookupFlags = enum(c_uint) { use_builtin = 0x04 };
pub const GTK_ICON_LOOKUP_USE_BUILTIN: GtkIconLookupFlags = .use_builtin;

pub const GtkDestDefaults = enum(c_uint) { all = 7 };
pub const GTK_DEST_DEFAULT_ALL: GtkDestDefaults = .all;

pub const GdkDragAction = enum(c_uint) { move = 8 };
pub const GDK_ACTION_MOVE: GdkDragAction = .move;

pub const GdkModifierType = enum(c_uint) { button1 = 0x100 };
pub const GDK_BUTTON1_MASK: GdkModifierType = .button1;

pub const GDK_BUTTON_SECONDARY: guint = 3;
pub const GDK_KEY_Escape: guint = 0xff1b;

pub const GTK_STYLE_PROVIDER_PRIORITY_APPLICATION: guint = 600;

pub const PangoEllipsizeMode = enum(c_int) { none = 0, start = 1, middle = 2, end = 3 };
pub const PANGO_ELLIPSIZE_END: PangoEllipsizeMode = .end;

pub const GCallback = *const anyopaque;

pub extern fn gtk_init(argc: ?*c_int, argv: ?*?[*:null]?[*:0]u8) void;
pub extern fn gtk_main() void;
pub extern fn gtk_main_quit() void;

pub extern fn gtk_window_new(t: GtkWindowType) ?*GtkWidget;
pub extern fn gtk_window_set_default_size(w: *GtkWindow, width: c_int, height: c_int) void;

pub extern fn gtk_box_new(orientation: GtkOrientation, spacing: c_int) ?*GtkWidget;
pub extern fn gtk_box_set_spacing(box: *GtkBox, spacing: c_int) void;
pub extern fn gtk_box_set_homogeneous(box: *GtkBox, homogeneous: gboolean) void;
pub extern fn gtk_box_pack_start(box: *GtkBox, child: *GtkWidget, expand: gboolean, fill: gboolean, padding: guint) void;

pub extern fn gtk_label_new(str: ?[*:0]const u8) ?*GtkWidget;
pub extern fn gtk_label_set_text(label: *GtkLabel, str: [*:0]const u8) void;
pub extern fn gtk_label_set_ellipsize(label: *GtkLabel, mode: PangoEllipsizeMode) void;

pub extern fn gtk_entry_new() ?*GtkWidget;
pub extern fn gtk_entry_get_text(entry: *GtkEntry) [*:0]const u8;
pub extern fn gtk_entry_set_text(entry: *GtkEntry, text: [*:0]const u8) void;

pub extern fn gtk_flow_box_new() ?*GtkWidget;
pub extern fn gtk_flow_box_child_new() ?*GtkWidget;
pub extern fn gtk_flow_box_set_homogeneous(box: *GtkFlowBox, homogeneous: gboolean) void;
pub extern fn gtk_flow_box_set_min_children_per_line(box: *GtkFlowBox, n: guint) void;
pub extern fn gtk_flow_box_set_max_children_per_line(box: *GtkFlowBox, n: guint) void;
pub extern fn gtk_flow_box_get_child_at_index(box: *GtkFlowBox, idx: c_int) ?*GtkFlowBoxChild;
pub extern fn gtk_flow_box_get_child_at_pos(box: *GtkFlowBox, x: c_int, y: c_int) ?*GtkFlowBoxChild;
pub extern fn gtk_flow_box_child_get_index(child: *GtkFlowBoxChild) c_int;

pub extern fn gtk_scrolled_window_new(hadjustment: ?*anyopaque, vadjustment: ?*anyopaque) ?*GtkWidget;
pub extern fn gtk_scrolled_window_set_policy(sw: *GtkScrolledWindow, hpolicy: GtkPolicyType, vpolicy: GtkPolicyType) void;

pub extern fn gtk_event_box_new() ?*GtkWidget;

pub extern fn gtk_menu_new() ?*GtkWidget;
pub extern fn gtk_menu_item_new() ?*GtkWidget;
pub extern fn gtk_separator_menu_item_new() ?*GtkWidget;
pub extern fn gtk_menu_shell_append(shell: *GtkMenuShell, child: *GtkWidget) void;
pub extern fn gtk_menu_popup_at_pointer(menu: *GtkMenu, trigger_event: ?*anyopaque) void;

pub extern fn gtk_container_add(container: *GtkContainer, widget: *GtkWidget) void;
pub extern fn gtk_container_remove(container: *GtkContainer, widget: *GtkWidget) void;
pub extern fn gtk_container_get_children(container: *GtkContainer) ?*GList;

pub extern fn gtk_widget_show(widget: *GtkWidget) void;
pub extern fn gtk_widget_show_all(widget: *GtkWidget) void;
pub extern fn gtk_widget_destroy(widget: *GtkWidget) void;
pub extern fn gtk_widget_set_visible(widget: *GtkWidget, visible: gboolean) void;
pub extern fn gtk_widget_set_size_request(widget: *GtkWidget, w: c_int, h: c_int) void;
pub extern fn gtk_widget_set_hexpand(widget: *GtkWidget, expand: gboolean) void;
pub extern fn gtk_widget_set_vexpand(widget: *GtkWidget, expand: gboolean) void;
pub extern fn gtk_widget_set_halign(widget: *GtkWidget, align_: GtkAlign) void;
pub extern fn gtk_widget_grab_focus(widget: *GtkWidget) void;
pub extern fn gtk_widget_activate(widget: *GtkWidget) gboolean;
pub extern fn gtk_widget_add_events(widget: *GtkWidget, events: c_int) void;
pub extern fn gtk_widget_get_style_context(widget: *GtkWidget) *GtkStyleContext;
pub extern fn gtk_widget_set_state_flags(widget: *GtkWidget, flags: GtkStateFlags, clear: gboolean) void;
pub extern fn gtk_widget_unset_state_flags(widget: *GtkWidget, flags: GtkStateFlags) void;

pub const GDK_BUTTON_PRESS_MASK: c_int = 256;
pub const GDK_ENTER_NOTIFY_MASK: c_int = 4096;
pub const GDK_LEAVE_NOTIFY_MASK: c_int = 8192;

pub extern fn gtk_style_context_add_class(ctx: *GtkStyleContext, class_name: [*:0]const u8) void;
pub extern fn gtk_style_context_remove_class(ctx: *GtkStyleContext, class_name: [*:0]const u8) void;
pub extern fn gtk_style_context_has_class(ctx: *GtkStyleContext, class_name: [*:0]const u8) gboolean;
pub extern fn gtk_style_context_add_provider(ctx: *GtkStyleContext, provider: *anyopaque, priority: guint) void;
pub extern fn gtk_style_context_add_provider_for_screen(screen: *GdkScreen, provider: *anyopaque, priority: guint) void;

pub extern fn gtk_css_provider_new() *GtkCssProvider;
pub extern fn gtk_css_provider_load_from_data(provider: *GtkCssProvider, data: [*:0]const u8, length: c_long, error_: ?*?*GError) gboolean;
pub extern fn gtk_css_provider_load_from_path(provider: *GtkCssProvider, path: [*:0]const u8, error_: ?*?*GError) gboolean;

pub extern fn gtk_icon_theme_get_default() *GtkIconTheme;
pub extern fn gtk_icon_theme_lookup_icon(theme: *GtkIconTheme, icon_name: [*:0]const u8, size: c_int, flags: GtkIconLookupFlags) ?*GtkIconInfo;
pub extern fn gtk_icon_info_get_filename(info: *GtkIconInfo) ?[*:0]const u8;

pub extern fn gtk_layer_init_for_window(window: *GtkWindow) void;
pub extern fn gtk_layer_set_layer(window: *GtkWindow, layer: GtkLayerShellLayer) void;
pub extern fn gtk_layer_set_keyboard_mode(window: *GtkWindow, mode: GtkLayerShellKeyboardMode) void;
pub extern fn gtk_layer_set_namespace(window: *GtkWindow, namespace: [*:0]const u8) void;

pub extern fn gdk_screen_get_default() *GdkScreen;

pub extern fn gtk_drag_source_set(widget: *GtkWidget, start_button_mask: GdkModifierType, targets: ?*anyopaque, n_targets: c_int, actions: GdkDragAction) void;
pub extern fn gtk_drag_dest_set(widget: *GtkWidget, flags: GtkDestDefaults, targets: ?*anyopaque, n_targets: c_int, actions: GdkDragAction) void;
pub extern fn gtk_drag_finish(context: *GdkDragContext, success: gboolean, del: gboolean, time_: guint32) void;
pub extern fn gtk_selection_data_get_length(data: *GtkSelectionData) gint;
pub extern fn gtk_selection_data_get_data(data: *GtkSelectionData) ?[*]const u8;
pub extern fn gtk_selection_data_get_target(data: *GtkSelectionData) *GdkAtom;
pub extern fn gtk_drag_set_icon_widget(context: *GdkDragContext, widget: *GtkWidget, hot_x: c_int, hot_y: c_int) void;
pub extern fn gtk_drag_set_icon_default(context: *GdkDragContext) void;

pub extern fn g_signal_connect_data(instance: *anyopaque, detailed_signal: [*:0]const u8, c_handler: GCallback, data: ?*anyopaque, destroy_data: ?*anyopaque, connect_flags: c_uint) gulong;
pub extern fn g_list_nth_data(list: ?*GList, n: guint) ?*anyopaque;
pub extern fn g_list_free(list: ?*GList) void;
pub extern fn g_object_unref(obj: *anyopaque) void;
pub extern fn g_error_free(err: *GError) void;

// Cairo
pub const cairo_surface_t = opaque {};
pub const cairo_t = opaque {};
pub const cairo_status_t = enum(c_uint) { success = 0, _ };
pub const cairo_format_t = enum(c_int) { argb32 = 0, rgb24 = 1, a8 = 2 };
pub const CAIRO_STATUS_SUCCESS: cairo_status_t = .success;
pub const CAIRO_FORMAT_ARGB32: cairo_format_t = .argb32;
pub const CAIRO_FORMAT_RGB24: cairo_format_t = .rgb24;

pub extern fn cairo_image_surface_create(format: cairo_format_t, width: c_int, height: c_int) ?*cairo_surface_t;
pub extern fn cairo_image_surface_create_from_png(filename: [*:0]const u8) ?*cairo_surface_t;
pub extern fn cairo_image_surface_get_width(surface: *cairo_surface_t) c_int;
pub extern fn cairo_image_surface_get_height(surface: *cairo_surface_t) c_int;
pub extern fn cairo_image_surface_get_stride(surface: *cairo_surface_t) c_int;
pub extern fn cairo_image_surface_get_data(surface: *cairo_surface_t) ?[*]u8;
pub extern fn cairo_surface_status(surface: *cairo_surface_t) cairo_status_t;
pub extern fn cairo_surface_destroy(surface: *cairo_surface_t) void;
pub extern fn cairo_surface_mark_dirty(surface: *cairo_surface_t) void;
pub extern fn cairo_create(target: *cairo_surface_t) ?*cairo_t;
pub extern fn cairo_destroy(cr: *cairo_t) void;
pub extern fn cairo_status(cr: *cairo_t) cairo_status_t;
pub extern fn cairo_scale(cr: *cairo_t, sx: f64, sy: f64) void;
pub extern fn cairo_set_source_surface(cr: *cairo_t, surface: *cairo_surface_t, x: f64, y: f64) void;
pub extern fn cairo_paint(cr: *cairo_t) void;

// Image surface creation — delegated to C shim (jpeg_create_decompress is a C macro)
pub extern fn shim_create_jpeg_surface(path: [*:0]const u8) ?*cairo_surface_t;
pub extern fn shim_create_webp_surface(path: [*:0]const u8) ?*cairo_surface_t;
pub extern fn shim_create_svg_surface(path: [*:0]const u8, w: c_int, h: c_int) ?*cairo_surface_t;

// dladdr for resolving .so path at runtime
pub const Dl_info = extern struct {
    dli_fname: ?[*:0]const u8,
    dli_fbase: ?*anyopaque,
    dli_sname: ?[*:0]const u8,
    dli_saddr: ?*anyopaque,
};
pub extern fn dladdr(addr: *const anyopaque, info: *Dl_info) c_int;

// Shim functions from gtk_shim.c
pub extern fn shim_container_clear(container: *anyopaque) void;
pub extern fn launcher_event_keyval(event: ?*anyopaque) guint;
pub extern fn launcher_event_is_secondary_button(event: ?*anyopaque) gboolean;
pub extern fn launcher_drag_source_set(w: *GtkWidget, button_mask: c_int, target_name: [*:0]const u8, info: guint, action: GdkDragAction) void;
pub extern fn launcher_drag_dest_set(w: *GtkWidget, target_name: [*:0]const u8, info: guint, action: GdkDragAction) void;
pub extern fn launcher_drag_motion(w: *GtkWidget, ctx: *GdkDragContext, x: c_int, y: c_int, time: guint, data: ?*anyopaque) gboolean;
pub extern fn launcher_drag_data_set_filename(sel: *GtkSelectionData, filename: [*]const u8, len: c_int) void;
pub extern fn launcher_selection_get_data(sel: *GtkSelectionData, len: *c_int) ?[*]const u8;
