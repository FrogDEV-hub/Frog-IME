// SPDX-License-Identifier: LGPL-2.1-or-later

#include "buffer.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <vector>

using frog_ime::Buffer;
using frog_ime::DaqianLayoutClassifier;
using frog_ime::InputState;
using frog_ime::KeyEvent;
using frog_ime::KeyResult;
using frog_ime::PhraseUnit;
using frog_ime::RimeBackend;
using frog_ime::SpecialKey;

namespace {

class FakeRimeBackend final : public RimeBackend {
public:
    std::map<std::string, std::vector<std::string>> dict = {
        {"su3", {"你", "擬"}},
        {"cl3", {"好"}},
        {"su3cl3", {"你好", "你", "擬"}},
        {"xl4", {"瀏"}},
        {"x94", {"覽"}},
        {"fup4", {"器"}},
        {"g84", {"搜"}},
        {"s4", {"尋"}},
        {"u3", {"引"}},
        {"r/4", {"擎"}},
        {"1u3", {"甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸", "子"}},
        {"j4", {"務"}},
        {"xu3", {"里"}},
        {"j4xu3", {"物理"}},
    };

    std::vector<PhraseUnit> last_learned;
    bool learn_result = true;

    std::vector<frog_ime::Candidate>
    convertSyllable(const std::string &keys) override {
        std::vector<frog_ime::Candidate> out;
        if (auto it = dict.find(keys); it != dict.end()) {
            for (const auto &s : it->second) {
                out.push_back({s});
            }
        }
        return out;
    }

    std::vector<std::string>
    candidatesAt(const std::string &reading_keys) override {
        if (auto it = dict.find(reading_keys); it != dict.end()) {
            return it->second;
        }
        return {};
    }

    std::string
    convertPhrase(const std::vector<std::string> &readings) override {
        std::string key;
        for (const auto &r : readings) {
            key += r;
        }
        if (auto it = dict.find(key); it != dict.end() && !it->second.empty()) {
            return it->second.front();
        }
        return {};
    }

    bool learnPhrase(const std::vector<PhraseUnit> &units) override {
        if (units.size() < 2) {
            return false;
        }
        last_learned = units;
        return learn_result;
    }
};

KeyResult typeString(Buffer &buf, const std::string &s) {
    KeyResult last;
    for (char c : s) {
        if (c == ' ') {
            last = buf.handleKey(KeyEvent{.special = SpecialKey::Space, .ch = ' '});
        } else if (c >= 'A' && c <= 'Z') {
            last = buf.handleKey(KeyEvent{.ch = c, .shift = true});
        } else {
            last = buf.handleKey(KeyEvent{.ch = c});
        }
    }
    return last;
}

} // namespace

TEST_CASE("Buffer displays Bopomofo symbols for pending untoned syllable",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    auto r1 = buf.handleKey(KeyEvent{.ch = 's'});
    REQUIRE(r1.handled);
    REQUIRE(r1.preedit == "ㄋ");

    auto r2 = buf.handleKey(KeyEvent{.ch = 'u'});
    REQUIRE(r2.handled);
    REQUIRE(r2.preedit == "ㄋㄧ");

    auto r3 = buf.handleKey(KeyEvent{.ch = '3'});
    REQUIRE(r3.handled);
    REQUIRE(r3.preedit == "你");
    REQUIRE(buf.pendingKeys().empty());
}

TEST_CASE("Buffer handles empty-queue English passthrough for 'google 搜尋引擎'",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // 'g' (ㄕ), 'o' (ㄟ), 'o' (slot conflict -> flush "goo" & EnglishPassthrough)
    REQUIRE(buf.handleKey(KeyEvent{.ch = 'g'}).preedit == "ㄕ");
    REQUIRE(buf.handleKey(KeyEvent{.ch = 'o'}).preedit == "ㄕㄟ");
    auto r_conflict = buf.handleKey(KeyEvent{.ch = 'o'});
    REQUIRE(r_conflict.handled);
    REQUIRE(r_conflict.commit_text == "goo");
    REQUIRE(r_conflict.preedit.empty());
    REQUIRE(buf.state() == InputState::EnglishPassthrough);

    // 'g', 'l', 'e' pass through directly
    REQUIRE_FALSE(buf.handleKey(KeyEvent{.ch = 'g'}).handled);
    REQUIRE_FALSE(buf.handleKey(KeyEvent{.ch = 'l'}).handled);
    REQUIRE_FALSE(buf.handleKey(KeyEvent{.ch = 'e'}).handled);

    // Space passes through and restores DetectingZhuyin
    auto r_space =
        buf.handleKey(KeyEvent{.special = SpecialKey::Space, .ch = ' '});
    REQUIRE_FALSE(r_space.handled);
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Next Chinese word "搜尋"
    typeString(buf, "g84s4");
    auto r_enter = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_enter.handled);
    REQUIRE(r_enter.commit_text == "搜尋");
}

TEST_CASE("Buffer handles Shift+A..Z on empty queue as immediate passthrough",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    auto r = buf.handleKey(KeyEvent{.ch = 'A', .shift = true});
    REQUIRE(r.handled);
    REQUIRE(r.commit_text == "A");
    REQUIRE(r.preedit.empty());
    REQUIRE(buf.state() == InputState::EnglishPassthrough);
}

TEST_CASE("Buffer keeps mixed Chinese and English in queue for '瀏覽器 Browser'",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // Type "瀏覽器"
    auto r_cn = typeString(buf, "xl4x94fup4");
    REQUIRE(r_cn.preedit == "瀏覽器");
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Space inserts literal space without opening candidate window
    auto r_sp =
        buf.handleKey(KeyEvent{.special = SpecialKey::Space, .ch = ' '});
    REQUIRE(r_sp.handled);
    REQUIRE(r_sp.preedit == "瀏覽器 ");
    REQUIRE(r_sp.candidates.empty());

    // Type "Browser" (starts with Shift+B)
    auto r_en = typeString(buf, "Browser");
    REQUIRE(r_en.preedit == "瀏覽器 Browser");
    REQUIRE(buf.state() == InputState::InQueueEnglish);

    // Space restores DetectingZhuyin
    auto r_sp2 =
        buf.handleKey(KeyEvent{.special = SpecialKey::Space, .ch = ' '});
    REQUIRE(r_sp2.preedit == "瀏覽器 Browser ");
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Enter commits the full mixed queue
    auto r_enter = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_enter.commit_text == "瀏覽器 Browser ");
    REQUIRE(r_enter.preedit.empty());
}

TEST_CASE("Buffer Backspace in InQueueEnglish restores DetectingZhuyin at Chinese boundary",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3");
    REQUIRE(buf.handleKey(KeyEvent{.ch = 'a'}).preedit == "你ㄇ");
    // 'c' conflicts with 'a' (both Initial) -> "你ac" in InQueueEnglish
    REQUIRE(buf.handleKey(KeyEvent{.ch = 'c'}).preedit == "你ac");
    REQUIRE(buf.state() == InputState::InQueueEnglish);

    buf.handleKey(KeyEvent{.special = SpecialKey::Backspace});
    REQUIRE(buf.state() == InputState::InQueueEnglish);
    auto r_bs = buf.handleKey(KeyEvent{.special = SpecialKey::Backspace});
    REQUIRE(r_bs.preedit == "你");
    REQUIRE(buf.state() == InputState::DetectingZhuyin);
}

TEST_CASE("Buffer handles Microsoft Bopomofo Shift+Punctuation and plain apostrophe",
          "[buffer]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // Plain apostrophe on empty queue passes through without candidates
    auto r_apos = buf.handleKey(KeyEvent{.ch = '\''});
    REQUIRE_FALSE(r_apos.handled);
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Shift+, on empty queue commits fullwidth comma directly
    auto r_comma = buf.handleKey(KeyEvent{.ch = ',', .shift = true});
    REQUIRE(r_comma.handled);
    REQUIRE(r_comma.commit_text == "，");
    REQUIRE(r_comma.preedit.empty());

    // Shift+, on non-empty queue appends fullwidth comma to queue
    typeString(buf, "su3");
    auto r_qcomma = buf.handleKey(KeyEvent{.ch = ',', .shift = true});
    REQUIRE(r_qcomma.handled);
    REQUIRE(r_qcomma.preedit == "你，");
    REQUIRE(r_qcomma.candidates.empty());
}

TEST_CASE("Buffer Down arrow opens candidate window and selects replacement",
          "[buffer][candidate]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3");
    REQUIRE(buf.handleKey(KeyEvent{.special = SpecialKey::Down}).candidates ==
            std::vector<std::string>{"你", "擬"});
    REQUIRE(buf.state() == InputState::CandidateOpen);

    // Move down to "擬" and confirm with Enter
    auto r_move = buf.handleKey(KeyEvent{.special = SpecialKey::Down});
    REQUIRE(r_move.highlighted_candidate == 1);

    auto r_sel = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_sel.handled);
    REQUIRE(r_sel.preedit == "擬");
    REQUIRE(r_sel.commit_text.empty());
    REQUIRE(r_sel.candidates.empty());
    REQUIRE(buf.state() == InputState::DetectingZhuyin);
}

TEST_CASE("Buffer candidate selection supports number keys 1-9 and paging",
          "[buffer][candidate]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3");
    REQUIRE(buf.handleKey(KeyEvent{.special = SpecialKey::Down}).candidates ==
            std::vector<std::string>{"你", "擬"});

    auto r_num = buf.handleKey(KeyEvent{.ch = '2'});
    REQUIRE(r_num.handled);
    REQUIRE(r_num.preedit == "擬");
    REQUIRE(r_num.candidates.empty());
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Paging: PageDown then '1' picks 10th candidate ("10")
    buf.reset();
    typeString(buf, "1u3");
    auto r_open = buf.handleKey(KeyEvent{.special = SpecialKey::Down});
    REQUIRE(r_open.candidates.size() == 11);
    auto r_pg = buf.handleKey(KeyEvent{.special = SpecialKey::PageDown});
    REQUIRE(r_pg.highlighted_candidate == 9);
    auto r_p2_1 = buf.handleKey(KeyEvent{.ch = '1'});
    REQUIRE(r_p2_1.preedit == "癸");
}

TEST_CASE("Buffer Down arrow targets character after cursor when cursor is inside preedit",
          "[buffer][candidate]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3cl3");
    // Cursor is at end (你好|); move Left once -> 你|好
    buf.handleKey(KeyEvent{.special = SpecialKey::Left});

    // Down should open candidates for "好" (cl3), NOT "你" (su3)
    auto r_down = buf.handleKey(KeyEvent{.special = SpecialKey::Down});
    REQUIRE(r_down.candidates == std::vector<std::string>{"好"});
}

TEST_CASE("Buffer offers phrase candidates and advances cursor to next character after selection",
          "[buffer][candidate][phrase]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3cl3");
    // Move cursor to start: |你好
    buf.handleKey(KeyEvent{.special = SpecialKey::Left});
    buf.handleKey(KeyEvent{.special = SpecialKey::Left});

    // Down at |你好 offers phrase candidate "你好" as well as single-char "你", "擬"
    auto r_open = buf.handleKey(KeyEvent{.special = SpecialKey::Down});
    REQUIRE(r_open.candidates == std::vector<std::string>{"你好", "你", "擬"});

    // Select "擬" (index 2 -> key '3'), cursor advances to 擬|好 so next Down targets "好"
    auto r_pick = buf.handleKey(KeyEvent{.ch = '3'});
    REQUIRE(r_pick.preedit == "擬好");
    auto r_next = buf.handleKey(KeyEvent{.special = SpecialKey::Down});
    REQUIRE(r_next.candidates == std::vector<std::string>{"好"});
}

TEST_CASE("Buffer restores DetectingZhuyin immediately after Shift+Punctuation in queue",
          "[buffer][punctuation]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3");
    // Enter InQueueEnglish with "ab"
    buf.handleKey(KeyEvent{.ch = 'a'});
    buf.handleKey(KeyEvent{.ch = 'b'});
    REQUIRE(buf.state() == InputState::InQueueEnglish);

    // Shift+, inserts ， and restores DetectingZhuyin
    auto r_p = buf.handleKey(KeyEvent{.ch = ',', .shift = true});
    REQUIRE(r_p.preedit == "你ab，");
    REQUIRE(buf.state() == InputState::DetectingZhuyin);

    // Next Zhuyin syllable must be detected as Chinese, not English
    auto r_zh = typeString(buf, "cl3");
    REQUIRE(r_zh.preedit == "你ab，好");
    REQUIRE(buf.state() == InputState::DetectingZhuyin);
}

TEST_CASE("Buffer auto-resegments queue into best phrase as new syllables are typed",
          "[buffer][phrase]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // j4 alone -> "務"
    auto r1 = typeString(buf, "j4");
    REQUIRE(r1.preedit == "務");

    // Adding xu3 ("里") auto-resegments j4+xu3 -> "物理"
    auto r2 = typeString(buf, "xu3");
    REQUIRE(r2.preedit == "物理");
}

TEST_CASE("Buffer Enter commits raw ASCII keys when untoned pending keys exist (2. or hi)",
          "[buffer][enter]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // "2." displays "ㄉㄡ" while pending, Enter outputs "2."
    auto r_dot = typeString(buf, "2.");
    REQUIRE(r_dot.preedit == "ㄉㄡ");
    auto r_enter1 = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_enter1.commit_text == "2.");
    REQUIRE(r_enter1.preedit.empty());

    // "hi" displays "ㄘㄛ" while pending, Enter outputs "hi"
    auto r_hi = typeString(buf, "hi");
    REQUIRE(r_hi.preedit == "ㄘㄛ");
    auto r_enter2 = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_enter2.commit_text == "hi");
    REQUIRE(r_enter2.preedit.empty());
}

TEST_CASE("Buffer Home and End move cursor within preedit queue without leaking to app",
          "[buffer][cursor]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // Empty queue: Home/End pass through to application
    REQUIRE_FALSE(buf.handleKey(KeyEvent{.special = SpecialKey::Home}).handled);
    REQUIRE_FALSE(buf.handleKey(KeyEvent{.special = SpecialKey::End}).handled);

    // Non-empty queue: Home moves to start, End moves to end
    typeString(buf, "su3cl3");
    auto r_home = buf.handleKey(KeyEvent{.special = SpecialKey::Home});
    REQUIRE(r_home.handled);
    REQUIRE(r_home.cursor_bytes == 0);
    // Down at cursor 0 targets "你" (first cell)
    REQUIRE(buf.handleKey(KeyEvent{.special = SpecialKey::Down}).candidates ==
            std::vector<std::string>{"你好", "你", "擬"});
    buf.handleKey(KeyEvent{.special = SpecialKey::Escape});

    auto r_end = buf.handleKey(KeyEvent{.special = SpecialKey::End});
    REQUIRE(r_end.handled);
    REQUIRE(r_end.cursor_bytes == static_cast<int>(std::string("你好").size()));
}

TEST_CASE("Buffer handles Ctrl+Left/Right within candidate list and preedit queue without leaking",
          "[buffer][cursor][candidate]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    // 1. In CandidateOpen, Ctrl+Right / Ctrl+Left moves candidate highlight
    typeString(buf, "su3");
    REQUIRE(buf.handleKey(KeyEvent{.special = SpecialKey::Down}).candidates ==
            std::vector<std::string>{"你", "擬"});
    auto r_cright =
        buf.handleKey(KeyEvent{.special = SpecialKey::Right, .ctrl = true});
    REQUIRE(r_cright.handled);
    REQUIRE(r_cright.highlighted_candidate == 1);
    auto r_cleft =
        buf.handleKey(KeyEvent{.special = SpecialKey::Left, .ctrl = true});
    REQUIRE(r_cleft.handled);
    REQUIRE(r_cleft.highlighted_candidate == 0);
    buf.handleKey(KeyEvent{.special = SpecialKey::Escape});

    // 2. In mixed preedit queue ("你 Browser"), Ctrl+Left jumps over "Browser"
    buf.reset();
    typeString(buf, "su3 Browser");
    auto r_jump =
        buf.handleKey(KeyEvent{.special = SpecialKey::Left, .ctrl = true});
    REQUIRE(r_jump.handled);
    REQUIRE(r_jump.cursor_bytes == static_cast<int>(std::string("你 ").size()));
}

TEST_CASE("Buffer Shift+Left/Right + Enter learns pure-Chinese phrase and stays in preedit",
          "[buffer][learn]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3cl3");
    REQUIRE(buf.cells().size() == 2);

    // Shift+Left twice selects "你好"
    auto s1 = buf.handleKey(KeyEvent{.special = SpecialKey::Left, .shift = true});
    REQUIRE(buf.state() == InputState::ShiftSelecting);
    REQUIRE(s1.selection_bytes.has_value());

    auto s2 = buf.handleKey(KeyEvent{.special = SpecialKey::Left, .shift = true});
    REQUIRE(s2.selection_bytes.has_value());
    REQUIRE(s2.selection_bytes->first == 0);
    REQUIRE(s2.selection_bytes->second ==
            static_cast<int>(std::string("你好").size()));

    // Enter learns phrase, does NOT commit, stays in preedit
    auto r_learn = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_learn.handled);
    REQUIRE(r_learn.commit_text.empty());
    REQUIRE(r_learn.preedit == "你好");
    REQUIRE(r_learn.notification.find("已加入詞庫：你好") != std::string::npos);
    REQUIRE(buf.state() == InputState::DetectingZhuyin);
    REQUIRE(backend.last_learned.size() == 2);
    REQUIRE(backend.last_learned[0].character == "你");
    REQUIRE(backend.last_learned[1].character == "好");

    // Second Enter commits the preedit
    auto r_commit = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r_commit.commit_text == "你好");
    REQUIRE(r_commit.preedit.empty());
}

TEST_CASE("Buffer Shift-selection rejects single char or mixed English/punctuation",
          "[buffer][learn]") {
    DaqianLayoutClassifier layout;
    FakeRimeBackend backend;
    Buffer buf(layout, backend);

    typeString(buf, "su3 ");
    // Select both "你" and " " (non-pure-Chinese)
    buf.handleKey(KeyEvent{.special = SpecialKey::Left, .shift = true});
    buf.handleKey(KeyEvent{.special = SpecialKey::Left, .shift = true});

    auto r = buf.handleKey(KeyEvent{.special = SpecialKey::Enter});
    REQUIRE(r.handled);
    REQUIRE(r.commit_text.empty());
    REQUIRE(r.preedit == "你 ");
    REQUIRE(r.notification == "僅支援框選純中文詞彙");
    REQUIRE(backend.last_learned.empty());
    REQUIRE(buf.state() == InputState::DetectingZhuyin);
}
