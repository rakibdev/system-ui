#!/bin/bash
cd "$(dirname "$0")"

export SHARE_DIR="$(cd .. && pwd)"

pkill -f "zig-out/bin/system-ui" 2>/dev/null
sleep 0.3

zig build && {
  cp ../extensions/launcher/default.css zig-out/lib/
  zig-out/bin/system-ui daemon &
  sleep 0.3
  zig-out/bin/system-ui zig-out/lib/liblauncher.so toggle
}
