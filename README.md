_System wide theming_

![](demo/system-wide-theming.png)

_Launcher_

![](demo/launcher.png)

_Panel_

![](demo/panel.png)

## Installation

Required

- gtk3
- gtk-layer-shell
- pipewire

Optional

- bluez
- networkmanager
- hyprland (night light)
- ttf-material-symbols-variable-git (icons)

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

### Create

See [demo extension](demo/extension/) and [built-in extensions](extensions/).

### Use

Put ".so" files in ~/.config/system-ui/extensions. Then

```
system-ui extension demo
```
