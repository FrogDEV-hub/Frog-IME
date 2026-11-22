// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

namespace frog_ime {

enum class SpecialKey {
    None,
    Space,
    Enter,
    Backspace,
    Escape,
    Tab,
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    PageUp,
    PageDown,
};

struct KeyEvent {
    SpecialKey special = SpecialKey::None;
    char ch = '\0';
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
};

} // namespace frog_ime
