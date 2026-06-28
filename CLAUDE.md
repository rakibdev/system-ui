## Dev

```bash
# build separately because xmake doesn't accept multiple targets
xmake build ui && xmake build launcher

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

## Gotchas

- Clang 22 ICE (exit code 139) when accessing a cross-module extern global
  directly inside an `export namespace` function body (e.g.
  `systemUiConfig.get().darkMode`). Workaround: assign to a local ref first.
  ```cpp
  // crashes
  if (systemUiConfig.get().darkMode) ...
  // works
  const auto& cfg = systemUiConfig.get();

  cfg.darkMode ? ...
  ```

```
gsettings set org.gtk.Settings.Debug enable-inspector-keybinding true
```
