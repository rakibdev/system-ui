![](demo/system-wide-theming.png)

You see a black dot at top right corner? Hovering it smoothly reveals the panel. For a seamless fullscreen experience. No more taskbar distractions!

### Panel

![](demo/panel.png)

### Launcher

![](demo/launcher.png)

## Installation

Required

- gtk3
- gtk-layer-shell
- pipewire

Optional

- bluez
- networkmanager
- hyprland (Night Light)
- ttf-material-symbols-variable-git (Icons)

```
xmake config --mode=release
```

```
xmake build
xmake install --admin
```

```
xmake uninstall --admin
```

## Extensions

Generally extensions can be seen as GTK windows with access to System UI framework APIs. Built-in launcher, panel are also extensions.

### Develop

For guidance on creating and debugging extensions, refer to a demo extension [window-preview.](link)

### Usage

Place compiled .so files in the following directory:

```
~/.config/system-ui/extensions
```
