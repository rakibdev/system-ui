## Dev

```bash
# build separately — xmake doesn't accept multiple targets
xmake build system-ui
xmake build launcher
xmake build panel

# regenerate compile_commands.json for LSP
xmake project -k compile_commands

# user starts daemon, don't do yourself
./build/ui --serve

# load extension (relative or absolute .so path)
./build/ui ./build/launcher.so
./build/ui ./build/panel.so

# toggle opens panel
./build/ui ./build/panel.so toggle
```

## Package

```bash
makepkg -si -f
```

## Logs

`journalctl --user -u system-ui -f`
