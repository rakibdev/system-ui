# Zig App Notes

## Layer Shell + Popup Menus

- `GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE` breaks popup menu interaction (hover works but clicks don't emit `activate`).
- Use `on_demand` keyboard mode for windows that need popup menus.

## Build & Run

```bash
cd zig-app
SHARE_DIR="/path/to/system-ui" ./run.sh
```

- `SHARE_DIR` points to repo root so daemon finds `src/default.css` (material icons font).
- `default.css` from `extensions/launcher/` is copied next to `.so` in `zig-out/lib/`.
- Toggle launcher: `zig-out/bin/system-ui zig-out/lib/liblauncher.so toggle`
- Stop daemon: `zig-out/bin/system-ui stop daemon`

## Drag and Drop

- **Do not use `GTK_DEST_DEFAULT_ALL`** for drop targets. It silently rejects drops unless a `drag-motion` handler explicitly calls `gdk_drag_status()` — without it GTK never commits the drop and `drag-data-received` never fires.
- Use `GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_HIGHLIGHT` and connect `drag-motion` to a handler that calls `gdk_drag_status(ctx, GDK_ACTION_MOVE, time)` and returns `TRUE`.
- `drag-enter` is not a valid GTK3 signal name — use `drag-motion` instead.
