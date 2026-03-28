Implement `extensions/input/`

## Features

- On Space, if the top match is highly confident, it replaces the word.
- If you hit Backspace immediately after an auto-correction, it'll undo the correction and opens a GTK popup with 5 fuzzy suggestions.
- Selection: Use up-down arrow Keys to select; Esc to close.
- Double-space auto adds full stop
- Banglish Mode: Every word is transliterated from banglish to bangla real-time on each word. Popup ensures fuzziness for transliteration too e.g. typing `s` have 3 possibilities: স, শ, ষ
- Emoji have higher priority. e.g. typing `laugh` autocorrects to emoji but user can backspace to use text.
- Don't autocorrect is misses threshold e.g. person name.
- Keep it minimal, zero latency, lowest ram usage and avoid 3rd party libs.
- Prefer extension code isolated as possible but use existing daemon for gtk popup, applying css. Support keep alive `src/extension.h` if needed.
- Use glass ui for suggestion popup like how panel loads extensions/panel/default.css

## Dataset:

See `extensions/input/dataset`
banglish.js - for banglish. simplify data structure into compact e.g. rules.
english.txt - top 10k words sorted by frequency high to low
emojis.txt - emoji and tags seperated by space
these are large files, read first few lines to understand format.

- add `build-assets` C++ script for generating memory-mapped sorted blobs if improves performance, simple symspell (distance 1 - make configurable with var)

## Peformance:

- Utilize mmap if make sense
- Load when typing and 30-second inactivity timer will auto release memory, unmaps etc.

## State

- src/config.h src/utils/storage.h - CONFIG_DIR `~/.config/system-ui/input.json`
- src/daemon.cpp src/extension.h - we can toggle input `enum Mode { Off, English, Banglish }` using `onRequest`. useful for `InputMethodTile`

## extensions/panel/main.cpp - Add the InputMethodTile to the panel.

- Icon: spellcheck (EN) / language (BN) / keyboard_hide (Off).
- Label: "Input Mode".
- Description: Current mode (e.g., "Bangla Phonetic").
- Clicking cycles mode.

## Inspiration:

- https://github.com/shuaib128/Banglish/blob/main/Phonetic/phonetic.js - `import { data } from "./data"` is banglish.js
- https://wayland.app/protocols/input-method-unstable-v2

https://github.com/first20hours/google-10000-english/blob/master/google-10000-english.txt
https://github.com/end-4/dots-hyprland/blob/main/dots/.config/hypr/hyprland/scripts/fuzzel-emoji.sh
