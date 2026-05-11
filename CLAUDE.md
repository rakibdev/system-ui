## Dev

```bash
xmake build system-ui
xmake build launcher
xmake build panel

# user starts daemon
./build/ui --serve

# load extension
./build/ui ./build/launcher.so
```

## Package

```bash
makepkg -si -f
```

## Logs

`journalctl --user -u system-ui -f`
