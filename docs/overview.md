```
system-ui/
├── .clang-format                          # Google style, no namespace fix, no reflow
├── .gitignore
├── .gitmodules                            # submodule: material-color-utilities
├── .vscode/
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
├── AGENTS.md
├── LICENSE
├── README.md
├── input.plan.md
├── run                                    # Bash helper: builds & runs extensions
├── theme-bin                              # Compiled theme binary
├── xmake.lua                              # Root build → targets: system-ui (shared) + ui (cli)
├── xmake.libs.lua                         # Build: material-color-utilities static lib
├── xmake.utils.lua                        # Shared build helpers (setup, set_build_dir, set_extension)
│
├── libs/
│   └── material-color-utilities/          # Git submodule — Google Material Color Utilities (C++)
│       ├── cpp/cam/
│       ├── cpp/quantize/
│       ├── cpp/score/
│       ├── cpp/utils/
│       └── ...
│
├── screenshots/
│   ├── cli-interface.png
│   ├── launcher.png
│   ├── panel.png
│   └── system-wide-theming.png
│
├── src/                                   # 🔥 Core library (libsystem-ui.so)
│   ├── main.cpp                           # CLI entrypoint
│   ├── config.h                           # Paths, Config struct, StorageManager extern
│   ├── config.cpp
│   ├── daemon.h                           # Daemon namespace (manager, request, initialize)
│   ├── daemon.cpp                         # Unix socket server + HTTP server (port 7780)
│   ├── element.h                          # 🎯 Widget hierarchy (50+ classes)
│   ├── element.cpp
│   ├── extension.h                        # Extension base + ExtensionManager (dlopen)
│   ├── extension.cpp                      # .so plugin loader
│   ├── style.h                            # Style (GtkCssProvider wrapper)
│   ├── theme.h                            # Theme::getCssVariables()
│   ├── default.css
│   ├── services/
│   │   ├── audio.h / audio.cpp            # PipeWire nodes
│   │   ├── bluetooth.h / bluetooth.cpp    # BlueZ D-Bus
│   │   ├── hyprland.h / hyprland.cpp      # Hyprland IPC
│   │   ├── media.h / media.cpp            # MPRIS D-Bus
│   │   ├── network.h / network.cpp        # NetworkManager D-Bus
│   │   └── notifications.h / notifications.cpp  # Desktop notifications D-Bus
│   └── utils/
│       ├── argparser.h                    # ArgParser (header-only)
│       ├── css.h / css.cpp                # CssManager (file watching)
│       ├── debounce.h / debounce.cpp      # Debounce utility
│       ├── event.h                        # Typed EventManager<EventData>
│       ├── file.h / file.cpp              # FileWatcher, File::resolve
│       ├── image.h / image.cpp            # Cairo surface loaders
│       ├── log.h / log.cpp                # Log with source_location
│       ├── run.h / run.cpp                # process helpers
│       ├── storage.h                      # StorageManager<T> (glaze JSON)
│       └── transition.h / transition.cpp  # PropertyTransition (easing)
│
├── extensions/                            # 🔌 Plugin ecosystem (.so files)
│   ├── panel/                             # System panel (top bar)
│   │   ├── xmake.lua                      # → shared lib "panel"
│   │   ├── main.h / main.cpp
│   │   ├── audio-dialog.h / audio-dialog.cpp
│   │   ├── media-controls.h / media-controls.cpp
│   │   ├── notifications.h / notifications.cpp
│   │   ├── default.css
│   │   └── assets/shaders/
│   │       ├── reset.frag
│   │       └── night-light.frag
│   ├── launcher/                          # App launcher
│   │   ├── xmake.lua                      # → shared lib "launcher"
│   │   ├── main.h / main.cpp
│   │   ├── drag-drop.h / drag-drop.cpp
│   │   └── default.css
│   ├── theme/                             # Material You color theme generator
│   │   ├── xmake.lua                      # → binary "theme"
│   │   ├── main.cpp
│   │   ├── theme.h / theme.cpp
│   │   ├── material.h / material.cpp
│   │   ├── color.h / color.cpp
│   │   ├── PKGBUILD
│   │   └── README.md
│   ├── media/                             # Media player binary
│   │   ├── xmake.lua                      # → binary "media"
│   │   └── main.cpp
│   ├── themed-icon/                       # Themed icon extension
│   │   ├── main.cpp
│   │   └── docs.json
│   └── windows/
│       └── .vscode/ (tasks.json, launch.json)
```

---

## 2. Build System Details

### 2.1 Root: `xmake.lua`

```lua
-- Target: shared library
target("system-ui")
    set_kind("shared")
    set_build_dir()
    add_files("src/**.cpp")
    add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3",
                  "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0")
    add_cxxflags("-fPIC")
    add_installfiles("src/*.h",           { prefixdir = "include/system-ui" })
    add_installfiles("src/services/*.h",  { prefixdir = "include/system-ui/services" })
    add_installfiles("src/utils/*.h",     { prefixdir = "include/system-ui/utils" })
    -- generates pkg-config file

-- Target: CLI binary
target("ui")
    set_build_dir()
    add_deps("system-ui")
    add_rpathdirs("@loader_path")
```

### 2.2 Shared Helpers: `xmake.utils.lua`

```lua
set_languages("c++26")          # C++23/26 (latest committee draft)
set_toolchains("clang")         # Clang-only
set_defaultmode("debug")
add_cxxflags("-Wno-absolute-value")

-- Debug mode: DEV define + SHARE_DIR = project root
-- Release mode: SHARE_DIR = /usr/share/system-ui
```

### 2.3 Extension Build Pattern: `set_extension(extName)`

```lua
function set_extension(extName)
    set_kind("shared")
    set_targetdir("build")
    add_cxxflags("-fPIC")
    add_linkdirs("../../build/")
    add_links("system-ui")
    set_prefixname("")           -- No "lib" prefix → "panel.so" not "libpanel.so"
    add_packages("gtk+-3.0", "gtk-layer-shell-0", "glaze")
end
```

### 2.4 Static Lib: `xmake.libs.lua`

```lua
target("material-color-utilities")
    set_kind("static")
    add_files("libs/.../cpp/utils/utils.cc")
    add_files("libs/.../cpp/cam/**.cc")
    add_files("libs/.../cpp/quantize/**.cc")
    add_files("libs/.../cpp/score/**.cc")
    remove_files(".../**_test.cc")
    add_includedirs("libs/material-color-utilities", {public = true})
    add_cxxflags("-fPIC")
```

### 2.5 All Dependencies

| Package | System | Used By |
|---------|--------|---------|
| `gtk+-3.0` | ✅ | Core, all extensions |
| `gtk-layer-shell-0` | ✅ | Core (Window), panel, launcher |
| `libpipewire-0.3` | ✅ | Core (audio service), panel |
| `glaze` | ✅ | Core (JSON config), launcher |
| `cairo` | ✅ | Core (image utils), theme |
| `libwebp` | ✅ | Core (image utils), theme |
| `libjpeg` | ✅ | Core (image utils), theme |
| `librsvg-2.0` | ✅ | Core (image utils), theme |
| `glib` | ✅ | media binary |

---

## 3. Module/Header Architecture

### 3.1 C++ Modules Check

| Keyword | Occurrences |
|---------|-------------|
| `export` | **0** ❌ |
| `import` | **0** ❌ |
| `module` | **0** ❌ |

> **The project uses traditional `#pragma once` headers throughout.** No C++20 modules are used despite the `c++26` language setting.

### 3.2 Precompiled Headers

| File | Status |
|------|--------|
| `*.pch` | **None** ❌ |
| `stdafx.h` / `stdafx.cpp` | **None** ❌ |
| PCH references in build files | **None** ❌ |

### 3.3 Header Count by Directory

| Directory | `.h` Files | `.cpp` Files |
|-----------|-----------|-------------|
| `src/` | 5 | 5 |
| `src/services/` | 6 | 6 |
| `src/utils/` | 10 | 7 |
| `extensions/panel/` | 4 | 4 |
| `extensions/launcher/` | 2 | 2 |
| `extensions/theme/` | 3 | 4 |
| **Total** | **30** `.h` | **28** `.cpp` (+ 2 `.cc` in lib) |

### 3.4 Key Header Relationships

```
main.cpp
├── config.h       → StorageManager<Config> (glaze JSON)
├── daemon.h       → extension.h
│   ├── extension.h  → Extension base + ExtensionManager
│   └── extension.cpp → dlfcn.h (dlopen/dlsym)
├── utils/argparser.h
└── utils/log.h    → source_location

element.h
├── style.h        → GtkCssProvider wrapper
├── gtk/gtk.h
├── gtk-layer-shell.h
└── (defines Box, Label, Button, Window, Menu, Transition, etc.)

extension.h (base for plugins)
├── panel/main.h   → Panel : Extension
├── launcher/main.h → Launcher : Extension
└── (loaded via dlsym "createExtension")
```

---

## 4. Core Architecture

### 4.1 Extension System (Plugin Architecture)

```
┌─────────────────────────────────────────┐
│            CLI (main.cpp)               │
│  "daemon" → Daemon::initialize()        │
│  "panel.so" → manager.load("panel.so")  │
│  "stop panel" → manager.unload("panel") │
└──────────────┬──────────────────────────┘
               │ Unix Socket
               ▼
┌─────────────────────────────────────────┐
│          Daemon (daemon.cpp)            │
│  - Unix socket: /tmp/system-ui/daemon.sock
│  - HTTP server: localhost:7780          │
│  - ExtensionManager manages .so plugins │
└──────────────┬──────────────────────────┘
               │ dlopen / dlsym
               ▼
┌─────────────────────────────────────────┐
│   Extension (extension.h)               │
│   virtual Response onRequest(command)   │
│                                         │
│   Panel.so    Launcher.so    ...        │
│   (shared)    (shared)                  │
│   Theme       Media                     │
│   (binary)    (binary)                  │
└─────────────────────────────────────────┘
```

**Extension lifecycle:**
1. `dlopen(path, RTLD_NOW)` — loads `.so`
2. `dlsym(handle, "createExtension")` — finds factory
3. `manager.add(path, unique_ptr<Extension>(createExtension()))` — stores
4. On unload: `extensions.erase(id)` (triggers destructor), then `dlclose`

**Macro for exporting:**
```cpp
#define EXPORT_EXTENSION(ExtensionClass) \
  extern "C" Extension* createExtension() { return new ExtensionClass() ; }
```

### 4.2 UI Widget Tree

```
Element (virtual destructor)
├── PointerEvents (mixin) — onPointerDown/Up
├── HoverEvents (mixin) — onHover/onHoverOut
├── ScrollEvents (mixin) — onScroll
├── KeyboardEvents (mixin) — onKeyDown
├── VisibilityEvents (mixin) — onHide
│
├── Box (HBox/VBox) — gap, spaceEvenly, prependChild
├── Label — set text
├── Icon (Box) — set icon name / image
├── Button — Text | Icon | IconText, Tonal | Filled, onClick
├── Input (KeyboardEvents) — value, placeholder, onChange/onSubmit
├── Slider (PointerEvents + ScrollEvents) — value (0-255), onChange
├── ScrolledWindow
├── FlowBox / FlowBoxChild — grid, columns, gap, onChildClick
├── EventBox (PointerEvents + HoverEvents + ScrollEvents + KeyboardEvents)
├── Window (EventBox) — GTK layer shell, align
├── Menu / MenuItem / MenuSeparator — popup menu
└── Transition — animated size (Frame {width, height}, easing)
```

### 4.3 Service Layer (D-Bus + PipeWire)

| Service | Technology | Provides |
|---------|-----------|----------|
| `Audio` | **PipeWire** | Sinks, sources, volume control, default device |
| `Bluetooth` | **D-Bus (BlueZ)** | Device discovery, connect/disconnect, battery |
| `Media` | **D-Bus (MPRIS)** | Players, play/pause/next/prev, progress |
| `Network` | **D-Bus (NetworkManager)** | Connection status, ethernet info |
| `Notifications` | **D-Bus (freedesktop)** | Notification list, actions, urgency |
| `Hyprland` | **IPC socket** | Window manager commands (e.g., night light) |

### 4.4 Utility Layer (Header-only highlights)

| Utility | Type | Description |
|---------|------|-------------|
| `ArgParser` | 🏗️ Header-only | Command-line argument parser |
| `StorageManager<T>` | 🏗️ Header-only | JSON file persistence via glaze |
| `EventManager` | 🏗️ Header-only | Typed event system (string + type_index keys) |
| `Style` | 🏗️ Header-only | GTK CSS provider wrapper |
| `Log` | ⚙️ + Header | Logging with `std::source_location` |
| `CssManager` | ⚙️ | CSS file watching + rebuild |
| `Debounce` | ⚙️ | Timer debouncing |
| `FileWatcher` | ⚙️ | GFileMonitor wrapper |
| `PropertyTransition` | ⚙️ | Value interpolation with easing |
| `Image` | ⚙️ | Cairo surface from WebP/JPEG/PNG/SVG |

### 4.5 Configuration & Themimg

```cpp
struct Config {
    bool darkMode = true ; bool watchFiles = true ; using Theme = std::unordered_map<std::string, std::string> ; Theme theme ; // Material You color map
} ; extern StorageManager<Config> systemUiConfig ; // Persisted to ~/.config/system-ui/system-ui.json
```

**Theme generation** (Material 3 / Material You):
1. Source color from wallpaper image
2. `MaterialColors::createDynamicPalette(sourceColor, dark)` produces:
   - `foreground`, `mutedForeground`, `background`, `card`, `popover`
   - `hover`, `primary`, `primaryForeground`, `secondary`, `secondaryForeground`, `border`
3. Exported as JSON + CSS custom properties (`@define-color`)

### 4.6 IPC Communication

**Unix Domain Socket** (primary):
- Path: `/tmp/system-ui/daemon.sock`
- JSON request/response format: `{"content": "...", "status": N}`
- CLI sends commands, daemon processes, returns response

**HTTP Server** (optional, `--serve` flag):
- Port: `localhost:7780`
- GET requests routed to loaded extensions by name
- Used for web-based control/debugging

---

## 5. Extension Details

### 5.1 Panel (`extensions/panel/`)

| File | Purpose |
|------|---------|
| `main.h/cpp` | Panel Extension — creates layer-shell window, audio/media/notifications sections |
| `audio-dialog.h/cpp` | Audio device selector popup |
| `media-controls.h/cpp` | MPRIS player widget (thumbnail, title, artist, slider) |
| `notifications.h/cpp` | Notification popups management |

**Submodules used:** `theme/theme.cpp`, `theme/color.cpp`, `theme/material.cpp`, `material-color-utilities`

### 5.2 Launcher (`extensions/launcher/`)

| File | Purpose |
|------|---------|
| `main.h/cpp` | Launcher Extension — app grid with search, pinning |
| `drag-drop.h/cpp` | Drag-and-drop for pinning/reordering apps |

**Data:** Reads `.desktop` files, caches in `AppCache` (JSON), supports context menu (`AppAction`).

### 5.3 Theme (`extensions/theme/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Standalone binary — extracts dominant color from image |
| `theme.h/cpp` | `colorFromImage()`, `generateJson()`, `generateCss()` |
| `material.h/cpp` | Material 3 dynamic palette creation (primary/secondary/surface/text variants) |
| `color.h/cpp` | Hex ↔ ARGB conversion, validation |

**Dependencies:** Cairo, WebP, JPEG, librsvg, material-color-utilities

### 5.4 Media (`extensions/media/`)

Simple binary that uses `MediaService` (MPRIS) to control media playback.

### 5.5 Themed Icon (`extensions/themed-icon/`)

Extension for rendering themed icon SVGs with dynamic colors.

---

## 6. Summary Table

| Aspect | Status |
|--------|--------|
| **C++ Standard** | 🟢 C++26 (`set_languages("c++26")`) |
| **Compiler** | 🟢 Clang |
| **Build System** | 🟢 xmake (Lua) |
| **C++20 Modules** | 🔴 Not used (0 occurrences of `export`/`import`/`module`) |
| **Precompiled Headers** | 🔴 None |
| **Header Style** | 🟢 `#pragma once` everywhere |
| **Plugin System** | 🟢 `dlopen`/`dlsym` shared library extensions |
| **IPC** | 🟢 Unix socket + HTTP server |
| **JSON** | 🟢 glaze library |
| **UI** | 🟢 GTK3 + gtk-layer-shell (Wayland) |
| **Audio** | 🟢 PipeWire |
| **D-Bus Services** | 🟢 BlueZ, MPRIS, NetworkManager, Notifications |
| **Theming** | 🟢 Material Color Utilities (Google) |
| **Testing** | 🔴 No test framework found |
| **Submodules** | 🟢 `material-color-utilities` (Git) |