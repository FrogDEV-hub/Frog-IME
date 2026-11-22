# Frog-IME (青蛙注音)

**English** | [繁體中文](README.zh-TW.md)

A Traditional Chinese Bopomofo (Zhuyin) input method for **Linux / Fcitx5**, clean-room implemented in C++20 with **`librime`** as the conversion backend and licensed under **LGPL-2.1-or-later**.

---

## Features

- **Modeless Mixed Chinese/English Input & Conditional Pre-edit Queue**
  - **Empty-queue English passthrough**: Typing words that collide on Daqian syllable slots (e.g., `google 搜尋引擎`) or starting with `Shift + A..Z` immediately flushes raw keys and passes through directly to the application until `Space` restores Zhuyin detection.
  - **In-queue mixed English**: Typing English inside an existing Chinese pre-edit queue (e.g., `瀏覽器 Browser`) keeps the English segment in the queue and restores Zhuyin detection on `Space` or when `Backspace` deletes back to the Chinese boundary.
- **Live Bopomofo Symbol Pre-edit**: Pending untoned syllables display Bopomofo symbols (e.g., `ㄋㄧ`) instead of raw ASCII keys. Pressing `Enter` while an untoned syllable is pending commits the raw ASCII text (e.g., `2.`, `hi`).
- **Automatic Phrase Conversion**: Uses `librime` (`bopomofo_tw` / `bopomofo`) for multi-syllable phrase conversion across contiguous Chinese cells, while preserving cells manually selected by the user.
- **Microsoft Bopomofo Compatible Ergonomics**:
  - `Space` acts as Tone-1 only when a pending syllable exists; otherwise it outputs a literal space without opening the candidate window.
  - Supports Microsoft Bopomofo `Shift + Punctuation` and `Ctrl` / `Ctrl + Shift` Chinese punctuation shortcuts.
- **`Shift`-Selection Phrase Learning**: Select contiguous pure-Chinese cells (≥ 2 characters) with `Shift + ←/→` (or `Shift + Home/End`) and press `Enter` to replay readings and learn the phrase into Rime's native `userdb` while staying in the pre-edit queue.

---

## Keybindings & Shortcuts

### Editing & Candidate Selection
| Key | Action |
|---|---|
| Daqian keys + `Space`/`3`/`4`/`6`/`7` | Complete syllable with Tone 1/3/4/2/neutral and convert to Chinese |
| `↓` | Open candidate window (offers both phrase and single-character candidates) |
| `1`–`9` / `Enter` / `Space` (in candidate window) | Select candidate and advance cursor to the next character |
| `↑`/`↓`/`←`/`→`, `PageUp`/`PageDown`, `Home`/`End` (in candidate window) | Navigate candidate highlight and pages |
| `Ctrl + ←` / `Ctrl + →` | Navigate candidates when candidate window is open, or jump word/cell within pre-edit queue |
| `Home` / `End` (non-empty queue) | Move cursor to start/end of pre-edit queue |
| `Shift + ←/→` (or `Shift + Home/End`) + `Enter` | Learn selected pure-Chinese phrase (≥ 2 chars) into Rime `userdb` without committing |
| `Enter` | Commit the entire pre-edit queue (untoned pending keys commit as raw ASCII) |

### Microsoft Bopomofo Chinese Punctuation Shortcuts
| Shortcut | Output | Shortcut | Output |
|---|---|---|---|
| `Ctrl + ,` / `Shift + ,` | `，` | `Ctrl + .` / `Shift + .` | `。` |
| `Ctrl + '` / `Shift + '` | `、` | `Ctrl + ;` | `；` |
| `Ctrl + [` / `Ctrl + ]` | `「` / `」` | `Shift + [` / `Shift + ]` | `『` / `』` |
| `Shift + ;` (`:`) / `Ctrl + Shift + ;` | `：` | `Shift + /` (`?`) / `Ctrl + Shift + /` | `？` |
| `Shift + 1` (`!`) / `Ctrl + Shift + 1` | `！` | `Shift + 9` / `Shift + 0` | `（` / `）` |
| `Shift + -` / `Shift + =` / `Shift + \` / ``Shift + ` `` | `—` / `＋` / `｜` / `～` | — | — |

---

## Requirements & Build / Install (Ubuntu / Debian)

### Dependencies
```bash
sudo apt install -y \
  cmake g++ pkg-config extra-cmake-modules \
  libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  librime-dev rime-data-bopomofo catch2
```

### Build & Install
> Note: On Ubuntu/Debian multiarch systems, pass `-DCMAKE_INSTALL_PREFIX=/usr` so the addon library installs into `/usr/lib/x86_64-linux-gnu/fcitx5/`.

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
fcitx5 -r -d
```

After restarting Fcitx5, open `fcitx5-configtool`, search for **`Frog Bopomofo (青蛙注音)`** under Available Input Methods, and add it to your active list.

---

## Testing

```bash
ctest --test-dir build --output-on-failure
```

Runs all 4 test suites: `smoke`, `layout`, `rime_backend`, and `buffer`.

---

## License & Third-Party Notices

This project is a clean-room implementation licensed under **LGPL-2.1-or-later** (see [LICENSE](LICENSE)). Interfaces and design patterns referenced from `fcitx5`, `fcitx5-rime`, `librime`, and `Catch2` are documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
