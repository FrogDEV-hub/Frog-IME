// SPDX-License-Identifier: LGPL-2.1-or-later

#include "layout.h"

#include <array>
#include <string_view>

namespace frog_ime {
namespace {

struct ZhuyinKeyEntry {
    char key;
    Slot slot;
    std::string_view symbol;
};

constexpr std::array<ZhuyinKeyEntry, 41> kDaqianTable{{
    // Initials (聲母, 21)
    {'1', Slot::Initial, "ㄅ"},
    {'q', Slot::Initial, "ㄆ"},
    {'a', Slot::Initial, "ㄇ"},
    {'z', Slot::Initial, "ㄈ"},
    {'2', Slot::Initial, "ㄉ"},
    {'w', Slot::Initial, "ㄊ"},
    {'s', Slot::Initial, "ㄋ"},
    {'x', Slot::Initial, "ㄌ"},
    {'e', Slot::Initial, "ㄍ"},
    {'d', Slot::Initial, "ㄎ"},
    {'c', Slot::Initial, "ㄏ"},
    {'r', Slot::Initial, "ㄐ"},
    {'f', Slot::Initial, "ㄑ"},
    {'v', Slot::Initial, "ㄒ"},
    {'5', Slot::Initial, "ㄓ"},
    {'t', Slot::Initial, "ㄔ"},
    {'g', Slot::Initial, "ㄕ"},
    {'b', Slot::Initial, "ㄖ"},
    {'y', Slot::Initial, "ㄗ"},
    {'h', Slot::Initial, "ㄘ"},
    {'n', Slot::Initial, "ㄙ"},
    // Medials (介音, 3)
    {'u', Slot::Medial, "ㄧ"},
    {'j', Slot::Medial, "ㄨ"},
    {'m', Slot::Medial, "ㄩ"},
    // Finals (韻母, 13)
    {'8', Slot::Final, "ㄚ"},
    {'i', Slot::Final, "ㄛ"},
    {'k', Slot::Final, "ㄜ"},
    {',', Slot::Final, "ㄝ"},
    {'9', Slot::Final, "ㄞ"},
    {'o', Slot::Final, "ㄟ"},
    {'l', Slot::Final, "ㄠ"},
    {'.', Slot::Final, "ㄡ"},
    {'0', Slot::Final, "ㄢ"},
    {'p', Slot::Final, "ㄣ"},
    {';', Slot::Final, "ㄤ"},
    {'/', Slot::Final, "ㄥ"},
    {'-', Slot::Final, "ㄦ"},
    // Tones (聲調, 4)
    {'3', Slot::Tone, "ˇ"},
    {'4', Slot::Tone, "ˋ"},
    {'6', Slot::Tone, "ˊ"},
    {'7', Slot::Tone, "˙"},
}};

struct PunctEntry {
    char unshifted;
    char shifted;
    std::string_view fullwidth;
};

constexpr std::array<PunctEntry, 14> kMsShiftPunctTable{{
    {',', '<', "，"},
    {'.', '>', "。"},
    {';', ':', "："},
    {'\'', '"', "、"},
    {'/', '?', "？"},
    {'1', '!', "！"},
    {'9', '(', "（"},
    {'0', ')', "）"},
    {'[', '{', "『"},
    {']', '}', "』"},
    {'\\', '|', "｜"},
    {'-', '_', "—"},
    {'=', '+', "＋"},
    {'`', '~', "～"},
}};

constexpr std::array<PunctEntry, 6> kCtrlPunctTable{{
    {',', '<', "，"},
    {'.', '>', "。"},
    {'\'', '"', "、"},
    {';', ':', "；"},
    {'[', '{', "「"},
    {']', '}', "」"},
}};

const ZhuyinKeyEntry *findDaqianEntry(char key) {
    for (const auto &entry : kDaqianTable) {
        if (entry.key == key) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

Slot DaqianLayoutClassifier::slotOf(const KeyEvent &ev) const {
    if (ev.special != SpecialKey::None || ev.shift || ev.ctrl || ev.alt) {
        return Slot::NonZhuyin;
    }
    if (const auto *entry = findDaqianEntry(ev.ch)) {
        return entry->slot;
    }
    return Slot::NonZhuyin;
}

std::string DaqianLayoutClassifier::toZhuyinSymbol(char key) const {
    if (const auto *entry = findDaqianEntry(key)) {
        return std::string(entry->symbol);
    }
    return {};
}

std::optional<std::string>
DaqianLayoutClassifier::msPunctuation(const KeyEvent &ev) const {
    if (ev.alt || ev.special != SpecialKey::None) {
        return std::nullopt;
    }
    if (ev.shift) {
        for (const auto &entry : kMsShiftPunctTable) {
            if (ev.ch == entry.unshifted || ev.ch == entry.shifted) {
                return std::string(entry.fullwidth);
            }
        }
        return std::nullopt;
    }
    if (ev.ctrl) {
        for (const auto &entry : kCtrlPunctTable) {
            if (ev.ch == entry.unshifted || ev.ch == entry.shifted) {
                return std::string(entry.fullwidth);
            }
        }
    }
    return std::nullopt;
}

} // namespace frog_ime
