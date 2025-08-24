## Build

#### Prerequisites:

- Install xmake.

#### Steps:

1. Configure if first build: `xmake config --mode=debug` or `xmake config --mode=release`

2. Press F1 > Run Task > Build.

Compiled .so file is copied to `~/.config/system-ui/extensions` automatically after each build. Refer to [xmake.lua](xmake.lua)

## Debug

#### Prerequisites:

- Install CodeLLDB extension in VS Code.

#### Steps:

1. Press F5 to execute [launch.json](.vscode/launch.json). This will trigger [build task, run daemon](.vscode/tasks.json) and attach debugger to it.
2. Set desired breakpoints (breakpoints work regardless of current your working directory).
3. Run the extension "system-ui run windows"
