https://github.com/rakibdev/material-code/assets/60188988/56918fe5-b8d2-4d2f-adba-7eebb99fdb3a

## Install

Required

- gtk3
- gtk-layer-shell
- pipewire

Optional

- bluez
- networkmanager
- hyprland (night light)

```
xmake config --mode=release
xmake build
xmake install (options: --installdir)
```

```
xmake uninstall (options: --installdir)
```

## Extensions

### Create

See [demo extension](demo/extension/) and [built-in extensions](extensions/).

### Use

Put .so files in ~/.config/system-ui/extensions. Then

```
system-ui daemon stop && system-ui daemon start
system-ui extension demo
```
