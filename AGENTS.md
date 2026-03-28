# Running Extensions

```bash
# build main lib first
xmake build system-ui

# start daemon
./build/ui daemon
# stop daemon
./build/ui stop daemon

# build & load extension
./run launcher
# unload extension
./run stop launcher

# dlopen sometimes doesn't clear cache, restart daemon if extension changes aren't reflected
./build/ui stop daemon && ./build/ui daemon &
```

# Build Extension

```bash
# -P specifies project directory, otherwise it builds root xmake.lua
xmake build -P extensions/<name>
```
