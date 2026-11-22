# Frog-IME（青蛙注音）

[English](README.md) | **繁體中文**

適用於 **Linux / Fcitx5** 的繁體中文大千注音輸入法（Clean-room C++20 實作，採用 **`librime`** 作為輸入法引擎，授權為 **LGPL-2.1-or-later**）。

---

## 功能特色

- **無模式中英混打與條件式組字隊列**
  - **空隊列英文直通**：如輸入 `google 搜尋引擎`（首字觸發大千槽位衝突）或以 `Shift + A..Z` 起頭時，直接透傳至應用程式，直到按下 `Space` 恢復注音偵測。
  - **隊列內中英混打**：如輸入 `瀏覽器 Browser` 時，保留英文段落於組字隊列中，按 `Space` 或以 `Backspace` 退回中文字格邊界時自動恢復注音偵測。
- **注音符號即時預覽**：未帶聲調之拼字過程直接顯示注音符號（如 `ㄋㄧ`），而非原始英數字母；若於未帶聲調狀態下直接按 `Enter`，則以原始英數字串輸出（如 `2.`、`hi`）。
- **整句／詞組自動選字**：以 `librime`（`bopomofo_tw` / `bopomofo`）進行多音節詞庫轉換，並保留使用者手動選過字的字格不被覆蓋。
- **微軟注音操作習慣相容**：
  - `Space` 僅在有待組音節時作為一聲；否則輸出空白，不誤開候選窗。
  - 支援微軟注音 `Shift + 標點` 與 `Ctrl` / `Ctrl + Shift` 中文標點快速鍵。
- **`Shift` 框選 + `Enter` 加入私有詞庫**：以 `Shift + ←/→`（或 `Shift + Home/End`）框選隊列中連續純中文詞彙（≥ 2 字）後按 `Enter`，將讀音與字詞重播寫入 Rime 原生 `userdb` 並停留於組字隊列。

---

## 操作與快捷鍵

### 基本編輯與選字
| 按鍵 | 行為 |
|---|---|
| 大千鍵位 + `Space`/`3`/`4`/`6`/`7` | 輸入聲調（一/三/四/二/輕聲）並轉為中文字格 |
| `↓` | 開啟候選清單（依游標位置提供詞組與單字候補） |
| `1`～`9` / `Enter` / `Space`（候選窗內） | 選取候選字並將游標推進至下一待選字 |
| `↑`/`↓`/`←`/`→`、`PageUp`/`PageDown`、`Home`/`End`（候選窗內） | 移動候選高亮與翻頁 |
| `Ctrl + ←` / `Ctrl + →` | 在候選窗內切換候選項目，或在隊列內逐單字／字格跳轉游標 |
| `Home` / `End`（隊列非空） | 移動游標至組字隊列開頭／尾端 |
| `Shift + ←/→`（或 `Shift + Home/End`）+ `Enter` | 框選純中文詞彙（≥ 2 字）寫入 Rime `userdb`（不送出組字區） |
| `Enter` | 送出整段組字隊列（若有未帶聲調音節則以原始按鍵字串送出） |

### 微軟注音中文標點快速鍵
| 快速鍵 | 輸出 | 快速鍵 | 輸出 |
|---|---|---|---|
| `Ctrl + ,` / `Shift + ,` | `，` | `Ctrl + .` / `Shift + .` | `。` |
| `Ctrl + '` / `Shift + '` | `、` | `Ctrl + ;` | `；` |
| `Ctrl + [` / `Ctrl + ]` | `「` / `」` | `Shift + [` / `Shift + ]` | `『` / `』` |
| `Shift + ;`（`:`） / `Ctrl + Shift + ;` | `：` | `Shift + /`（`?`） / `Ctrl + Shift + /` | `？` |
| `Shift + 1`（`!`） / `Ctrl + Shift + 1` | `！` | `Shift + 9` / `Shift + 0` | `（` / `）` |
| `Shift + -` / `Shift + =` / `Shift + \` / ``Shift + ` `` | `—` / `＋` / `｜` / `～` | — | — |

---

## 系統需求與建置安裝（Ubuntu / Debian）

### 相依套件
```bash
sudo apt install -y \
  cmake g++ pkg-config extra-cmake-modules \
  libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  librime-dev rime-data-bopomofo catch2
```

### 建置與安裝
> 注意：Ubuntu multiarch 架構需指定 `-DCMAKE_INSTALL_PREFIX=/usr`，以將插件安裝至 `/usr/lib/x86_64-linux-gnu/fcitx5/`。

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
fcitx5 -r -d
```

安裝後於 `fcitx5-configtool` 的「可用輸入法」搜尋 **`Frog Bopomofo (青蛙注音)`** 並加入使用中清單即可。

---

## 測試

```bash
ctest --test-dir build --output-on-failure
```

包含 4 組測試套件：`smoke`、`layout`、`rime_backend`、`buffer`。

---

## 授權與第三方聲明

本專案以 **LGPL-2.1-or-later** 授權條款進行 Clean-room 實作，詳見 [LICENSE](LICENSE)；參考自 `fcitx5`、`fcitx5-rime`、`librime` 與 `Catch2` 之介面與設計規範皆明確標註於 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
