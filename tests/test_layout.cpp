// SPDX-License-Identifier: LGPL-2.1-or-later

#include "layout.h"

#include <catch2/catch_test_macros.hpp>

using frog_ime::DaqianLayoutClassifier;
using frog_ime::KeyEvent;
using frog_ime::Slot;
using frog_ime::SpecialKey;

TEST_CASE("DaqianLayoutClassifier classifies all 41 Daqian keys into slots",
          "[layout]") {
    DaqianLayoutClassifier layout;

    SECTION("Initials (聲母)") {
        const char initials[] = "1qaz2wsxedcrfv5tgbyhn";
        for (char c : std::string(initials)) {
            INFO("key: " << c);
            REQUIRE(layout.slotOf(KeyEvent{.ch = c}) == Slot::Initial);
        }
    }

    SECTION("Medials (介音)") {
        for (char c : std::string("ujm")) {
            INFO("key: " << c);
            REQUIRE(layout.slotOf(KeyEvent{.ch = c}) == Slot::Medial);
        }
    }

    SECTION("Finals (韻母)") {
        for (char c : std::string("8ik,9ol.0p;/-")) {
            INFO("key: " << c);
            REQUIRE(layout.slotOf(KeyEvent{.ch = c}) == Slot::Final);
        }
    }

    SECTION("Tones (聲調)") {
        for (char c : std::string("3467")) {
            INFO("key: " << c);
            REQUIRE(layout.slotOf(KeyEvent{.ch = c}) == Slot::Tone);
        }
    }

    SECTION("Non-Zhuyin keys and modifiers") {
        REQUIRE(layout.slotOf(KeyEvent{.ch = '\''}) == Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = '['}) == Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = ']'}) == Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = '='}) == Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.special = SpecialKey::Space, .ch = ' '}) ==
                Slot::NonZhuyin);
        // Shift + A..Z must be NonZhuyin (Q11)
        REQUIRE(layout.slotOf(KeyEvent{.ch = 'A', .shift = true}) ==
                Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = 'a', .shift = true}) ==
                Slot::NonZhuyin);
        // Shift + punctuation is NonZhuyin
        REQUIRE(layout.slotOf(KeyEvent{.ch = ',', .shift = true}) ==
                Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = '<', .shift = true}) ==
                Slot::NonZhuyin);
        // Ctrl / Alt modified keys are NonZhuyin
        REQUIRE(layout.slotOf(KeyEvent{.ch = 'a', .ctrl = true}) ==
                Slot::NonZhuyin);
        REQUIRE(layout.slotOf(KeyEvent{.ch = 'u', .alt = true}) ==
                Slot::NonZhuyin);
    }
}

TEST_CASE("DaqianLayoutClassifier maps ASCII keys to Bopomofo symbols",
          "[layout]") {
    DaqianLayoutClassifier layout;

    REQUIRE(layout.toZhuyinSymbol('1') == "ㄅ");
    REQUIRE(layout.toZhuyinSymbol('s') == "ㄋ");
    REQUIRE(layout.toZhuyinSymbol('u') == "ㄧ");
    REQUIRE(layout.toZhuyinSymbol('j') == "ㄨ");
    REQUIRE(layout.toZhuyinSymbol('m') == "ㄩ");
    REQUIRE(layout.toZhuyinSymbol(',') == "ㄝ");
    REQUIRE(layout.toZhuyinSymbol('.') == "ㄡ");
    REQUIRE(layout.toZhuyinSymbol('/') == "ㄥ");
    REQUIRE(layout.toZhuyinSymbol(';') == "ㄤ");
    REQUIRE(layout.toZhuyinSymbol('-') == "ㄦ");
    REQUIRE(layout.toZhuyinSymbol('3') == "ˇ");
    REQUIRE(layout.toZhuyinSymbol('4') == "ˋ");
    REQUIRE(layout.toZhuyinSymbol('6') == "ˊ");
    REQUIRE(layout.toZhuyinSymbol('7') == "˙");
    REQUIRE(layout.toZhuyinSymbol('\'') == "");
    REQUIRE(layout.toZhuyinSymbol('A') == "");
}

TEST_CASE("DaqianLayoutClassifier maps Microsoft Bopomofo Shift+Punctuation",
          "[layout]") {
    DaqianLayoutClassifier layout;

    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ',', .shift = true}) == "，");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '<', .shift = true}) == "，");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '.', .shift = true}) == "。");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '>', .shift = true}) == "。");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ';', .shift = true}) == "：");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ':', .shift = true}) == "：");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '\'', .shift = true}) == "、");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '"', .shift = true}) == "、");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '/', .shift = true}) == "？");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '?', .shift = true}) == "？");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '1', .shift = true}) == "！");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '!', .shift = true}) == "！");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '(', .shift = true}) == "（");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ')', .shift = true}) == "）");

    // Unshifted keys must NOT trigger msPunctuation
    REQUIRE_FALSE(layout.msPunctuation(KeyEvent{.ch = ',', .shift = false})
                      .has_value());
    REQUIRE_FALSE(layout.msPunctuation(KeyEvent{.ch = '\'', .shift = false})
                      .has_value());
    // Shift+A..Z must NOT trigger msPunctuation
    REQUIRE_FALSE(layout.msPunctuation(KeyEvent{.ch = 'A', .shift = true})
                      .has_value());

    // Ctrl + punctuation (Microsoft Bopomofo standard)
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ',', .ctrl = true}) == "，");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '.', .ctrl = true}) == "。");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '\'', .ctrl = true}) == "、");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ';', .ctrl = true}) == "；");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = '[', .ctrl = true}) == "「");
    REQUIRE(layout.msPunctuation(KeyEvent{.ch = ']', .ctrl = true}) == "」");

    // Ctrl + Shift + punctuation
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = ',', .shift = true, .ctrl = true}) == "，");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = '<', .shift = true, .ctrl = true}) == "，");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = '.', .shift = true, .ctrl = true}) == "。");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = ':', .shift = true, .ctrl = true}) == "：");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = '?', .shift = true, .ctrl = true}) == "？");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = '!', .shift = true, .ctrl = true}) == "！");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = '[', .shift = true, .ctrl = true}) == "『");
    REQUIRE(layout.msPunctuation(
                KeyEvent{.ch = ']', .shift = true, .ctrl = true}) == "』");
}
