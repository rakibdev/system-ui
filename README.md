![](screenshots/system-wide-theming.png)

Notice a black dot at top right corner? Hovering it smoothly reveals the panel. For a seamless fullscreen experience. No more taskbar distractions.

### Panel

![](screenshots/panel.png)

### Launcher

![](screenshots/launcher.png)

### CLI Interface

![](screenshots/cli-interface.png)

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

Generally extensions can be seen as GTK windows with access to System UI framework APIs.

### Develop

For guidance on creating and debugging extensions, refer to a [system-ui-extensions](link) repo.

### Usage

Place compiled .so files in the following directory:

```
~/.config/system-ui/extensions
```
