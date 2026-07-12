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

## Module partitions

Some modules split into `X.cppm` (interface, exports only) +
`X.impl.cppm` (`module X:impl;`, implementation) to avoid full BMI
rebuild cascade on every importer when only implementation changes.
xmake invalidates BMIs by mtime, not content-hash, so editing a
module's interface (even a no-op touch) still forces all importers
to recompile — this split only protects the impl-only edit case.

Only worth splitting modules with real fan-in (multiple importers)
AND separable implementation (free functions/logic, not just a class
with inline method bodies — a struct's inline methods are the
exported interface itself, nothing to hide in a partition).

Currently split: `apps` (extensions/launcher), `daemon`, `log`.
Skipped: `elements.*` (pure class shape, no separable impl), `config`
(trivial constants), leaf modules with single importer (e.g. `ui` in
launcher — no downstream BMI to protect).

Implementation partitions must explicitly `import` their own primary
module to see its exports (not automatic).

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
