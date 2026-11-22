// SPDX-License-Identifier: LGPL-2.1-or-later

#include "buffer.h"

#include <algorithm>
#include <cctype>

namespace frog_ime {
namespace {

std::vector<std::string> splitUtf8Chars(const std::string &s) {
    std::vector<std::string> chars;
    for (std::size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        std::size_t len = 1;
        if ((b & 0xe0) == 0xc0) {
            len = 2;
        } else if ((b & 0xf0) == 0xe0) {
            len = 3;
        } else if ((b & 0xf8) == 0xf0) {
            len = 4;
        }
        len = std::min(len, s.size() - i);
        chars.push_back(s.substr(i, len));
        i += len;
    }
    return chars;
}

int slotOrder(Slot s) {
    switch (s) {
    case Slot::Initial:
        return 0;
    case Slot::Medial:
        return 1;
    case Slot::Final:
        return 2;
    case Slot::Tone:
        return 3;
    case Slot::NonZhuyin:
        return -1;
    }
    return -1;
}

bool isPrintableAscii(const KeyEvent &ev) {
    return ev.special == SpecialKey::None && !ev.ctrl && !ev.alt &&
           ev.ch >= 0x20 && ev.ch <= 0x7e;
}

bool isShiftUppercaseAlpha(const KeyEvent &ev) {
    return ev.special == SpecialKey::None && !ev.ctrl && !ev.alt && ev.shift &&
           std::isalpha(static_cast<unsigned char>(ev.ch)) != 0;
}

char toUpperAscii(char ch) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
}

} // namespace

Buffer::Buffer(const LayoutClassifier &layout, RimeBackend &backend)
    : layout_(layout), backend_(backend) {}

void Buffer::reset() {
    state_ = InputState::DetectingZhuyin;
    cells_.clear();
    cursor_idx_ = 0;
    pending_keys_.clear();
    max_pending_slot_ = -1;
    candidates_.clear();
    highlighted_candidate_ = 0;
    candidate_target_idx_ = 0;
    sel_anchor_ = 0;
    sel_focus_ = 0;
}

std::string Buffer::pendingZhuyin() const {
    std::string out;
    for (char k : pending_keys_) {
        out += layout_.toZhuyinSymbol(k);
    }
    return out;
}

KeyResult Buffer::makePreeditResult(bool handled, std::string commit,
                                    std::string notification) const {
    KeyResult res;
    res.handled = handled;
    res.commit_text = std::move(commit);
    res.notification = std::move(notification);

    std::string pending = pendingZhuyin();
    int cursor = 0;
    for (std::size_t i = 0; i < cursor_idx_ && i < cells_.size(); ++i) {
        res.preedit += cells_[i].text;
    }
    res.preedit += pending;
    cursor = static_cast<int>(res.preedit.size());
    for (std::size_t i = cursor_idx_; i < cells_.size(); ++i) {
        res.preedit += cells_[i].text;
    }
    res.cursor_bytes = cursor;

    if (state_ == InputState::CandidateOpen) {
        res.candidates = candidates_;
        res.highlighted_candidate = highlighted_candidate_;
    }

    if (state_ == InputState::ShiftSelecting) {
        std::size_t lo = std::min(sel_anchor_, sel_focus_);
        std::size_t hi = std::max(sel_anchor_, sel_focus_);
        int start_b = 0;
        for (std::size_t i = 0; i < lo && i < cells_.size(); ++i) {
            start_b += static_cast<int>(cells_[i].text.size());
        }
        if (lo <= cursor_idx_ && !pending.empty()) {
            start_b += static_cast<int>(pending.size());
        }
        int end_b = start_b;
        for (std::size_t i = lo; i < hi && i < cells_.size(); ++i) {
            end_b += static_cast<int>(cells_[i].text.size());
        }
        res.selection_bytes = std::pair{start_b, end_b};
    }

    return res;
}

void Buffer::insertRawToQueue(const std::string &raw) {
    for (char c : raw) {
        cells_.insert(cells_.begin() + static_cast<std::ptrdiff_t>(cursor_idx_),
                      Cell{CellKind::EnglishOrSpace, std::string(1, c), ""});
        ++cursor_idx_;
    }
}

void Buffer::flushPendingAsRaw(const std::string &extra, bool enter_english_mode,
                               KeyResult &out) {
    std::string raw = pending_keys_ + extra;
    pending_keys_.clear();
    max_pending_slot_ = -1;

    if (cells_.empty()) {
        out.handled = true;
        out.commit_text = raw;
        if (enter_english_mode) {
            state_ = InputState::EnglishPassthrough;
        }
        return;
    }

    insertRawToQueue(raw);
    if (enter_english_mode) {
        state_ = InputState::InQueueEnglish;
    }
    out = makePreeditResult(true);
}

void Buffer::updateInQueueEnglishAfterBackspace() {
    if (cells_.empty()) {
        state_ = InputState::DetectingZhuyin;
        return;
    }
    if (cursor_idx_ > 0 &&
        cells_[cursor_idx_ - 1].kind != CellKind::EnglishOrSpace) {
        state_ = InputState::DetectingZhuyin;
    }
}

void Buffer::resegmentAround(std::size_t idx) {
    if (idx >= cells_.size() || cells_[idx].kind != CellKind::Chinese) {
        return;
    }
    std::size_t run_start = idx;
    while (run_start > 0 &&
           cells_[run_start - 1].kind == CellKind::Chinese) {
        --run_start;
    }
    std::size_t run_end = idx + 1;
    while (run_end < cells_.size() &&
           cells_[run_end].kind == CellKind::Chinese) {
        ++run_end;
    }
    if (run_end - run_start < 2) {
        return;
    }

    std::vector<std::string> readings;
    readings.reserve(run_end - run_start);
    for (std::size_t i = run_start; i < run_end; ++i) {
        readings.push_back(cells_[i].reading_keys);
    }
    const std::string converted = backend_.convertPhrase(readings);
    const auto chars = splitUtf8Chars(converted);
    if (chars.size() != (run_end - run_start)) {
        return;
    }
    for (std::size_t i = 0; i < chars.size(); ++i) {
        if (!cells_[run_start + i].user_selected) {
            cells_[run_start + i].text = chars[i];
        }
    }
}

void Buffer::jumpCursorByWord(bool left) {
    if (left) {
        while (cursor_idx_ > 0 && cells_[cursor_idx_ - 1].text == " ") {
            --cursor_idx_;
        }
        if (cursor_idx_ == 0) {
            return;
        }
        if (cells_[cursor_idx_ - 1].kind == CellKind::EnglishOrSpace) {
            while (cursor_idx_ > 0 &&
                   cells_[cursor_idx_ - 1].kind == CellKind::EnglishOrSpace &&
                   cells_[cursor_idx_ - 1].text != " ") {
                --cursor_idx_;
            }
        } else {
            --cursor_idx_;
        }
        return;
    }

    if (cursor_idx_ >= cells_.size()) {
        return;
    }
    if (cells_[cursor_idx_].kind == CellKind::EnglishOrSpace &&
        cells_[cursor_idx_].text != " ") {
        while (cursor_idx_ < cells_.size() &&
               cells_[cursor_idx_].kind == CellKind::EnglishOrSpace &&
               cells_[cursor_idx_].text != " ") {
            ++cursor_idx_;
        }
    } else {
        ++cursor_idx_;
    }
    while (cursor_idx_ < cells_.size() && cells_[cursor_idx_].text == " ") {
        ++cursor_idx_;
    }
}

bool Buffer::tryOpenCandidates() {
    if (cells_.empty() || !pending_keys_.empty()) {
        return false;
    }
    std::size_t target = cells_.size();
    if (cursor_idx_ < cells_.size() &&
        cells_[cursor_idx_].kind == CellKind::Chinese) {
        target = cursor_idx_;
    } else if (cursor_idx_ > 0 &&
               cells_[cursor_idx_ - 1].kind == CellKind::Chinese) {
        target = cursor_idx_ - 1;
    } else {
        return false;
    }

    constexpr std::size_t kMaxPhraseCells = 6;
    std::string phrase_reading;
    for (std::size_t i = target;
         i < cells_.size() && (i - target) < kMaxPhraseCells; ++i) {
        if (cells_[i].kind != CellKind::Chinese ||
            cells_[i].reading_keys.empty()) {
            break;
        }
        phrase_reading += cells_[i].reading_keys;
    }

    auto cands = backend_.candidatesAt(phrase_reading);
    if (phrase_reading != cells_[target].reading_keys) {
        auto single_cands = backend_.candidatesAt(cells_[target].reading_keys);
        for (auto &sc : single_cands) {
            if (std::find(cands.begin(), cands.end(), sc) == cands.end()) {
                cands.push_back(std::move(sc));
            }
        }
    }
    if (cands.empty()) {
        return false;
    }

    state_before_candidate_ = state_;
    state_ = InputState::CandidateOpen;
    candidate_target_idx_ = target;
    candidates_ = std::move(cands);
    highlighted_candidate_ = 0;
    return true;
}

KeyResult Buffer::handleCandidateKey(const KeyEvent &ev) {
    constexpr int kPageSize = 9;
    const int n = static_cast<int>(candidates_.size());

    if (ev.special == SpecialKey::Escape) {
        state_ = state_before_candidate_;
        candidates_.clear();
        highlighted_candidate_ = 0;
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::Down || ev.special == SpecialKey::Right) {
        if (n > 0) {
            highlighted_candidate_ = (highlighted_candidate_ + 1) % n;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::Up || ev.special == SpecialKey::Left) {
        if (n > 0) {
            highlighted_candidate_ = (highlighted_candidate_ - 1 + n) % n;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::Home) {
        if (n > 0) {
            highlighted_candidate_ = 0;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::End) {
        if (n > 0) {
            highlighted_candidate_ = n - 1;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::PageDown) {
        if (n > 0) {
            const int next_page = (highlighted_candidate_ / kPageSize + 1) * kPageSize;
            highlighted_candidate_ = (next_page < n) ? next_page : 0;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::PageUp) {
        if (n > 0) {
            const int prev_page = (highlighted_candidate_ / kPageSize - 1) * kPageSize;
            highlighted_candidate_ = (prev_page >= 0) ? prev_page : 0;
        }
        return makePreeditResult(true);
    }

    int chosen = -1;
    if (ev.special == SpecialKey::Enter || ev.special == SpecialKey::Space) {
        chosen = highlighted_candidate_;
    } else if (!ev.shift && ev.ch >= '1' && ev.ch <= '9') {
        const int page_start = (highlighted_candidate_ / kPageSize) * kPageSize;
        chosen = page_start + (ev.ch - '1');
    }

    if (chosen >= 0 && chosen < n) {
        const auto chars =
            splitUtf8Chars(candidates_[static_cast<std::size_t>(chosen)]);
        const std::size_t span = chars.size();
        if (span >= 1 && candidate_target_idx_ + span <= cells_.size()) {
            for (std::size_t i = 0; i < span; ++i) {
                cells_[candidate_target_idx_ + i].text = chars[i];
                cells_[candidate_target_idx_ + i].user_selected = true;
            }
            cursor_idx_ = candidate_target_idx_ + span;
        }
        state_ = state_before_candidate_;
        candidates_.clear();
        highlighted_candidate_ = 0;
        return makePreeditResult(true);
    }

    return makePreeditResult(true);
}

KeyResult Buffer::handleShiftSelectKey(const KeyEvent &ev) {
    if (ev.shift && ev.special == SpecialKey::Left) {
        if (sel_focus_ > 0) {
            --sel_focus_;
            cursor_idx_ = sel_focus_;
        }
        return makePreeditResult(true);
    }
    if (ev.shift && ev.special == SpecialKey::Right) {
        if (sel_focus_ < cells_.size()) {
            ++sel_focus_;
            cursor_idx_ = sel_focus_;
        }
        return makePreeditResult(true);
    }
    if (ev.shift && ev.special == SpecialKey::Home) {
        sel_focus_ = 0;
        cursor_idx_ = 0;
        return makePreeditResult(true);
    }
    if (ev.shift && ev.special == SpecialKey::End) {
        sel_focus_ = cells_.size();
        cursor_idx_ = cells_.size();
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::Enter) {
        std::size_t lo = std::min(sel_anchor_, sel_focus_);
        std::size_t hi = std::max(sel_anchor_, sel_focus_);
        state_ = InputState::DetectingZhuyin;

        bool valid = (hi - lo >= 2) && (hi <= cells_.size());
        std::vector<PhraseUnit> units;
        std::string phrase_text;
        for (std::size_t i = lo; valid && i < hi; ++i) {
            if (cells_[i].kind != CellKind::Chinese ||
                cells_[i].reading_keys.empty()) {
                valid = false;
                break;
            }
            units.push_back(PhraseUnit{cells_[i].reading_keys, cells_[i].text});
            phrase_text += cells_[i].text;
        }

        if (!valid) {
            return makePreeditResult(true, {}, "僅支援框選純中文詞彙");
        }
        if (backend_.learnPhrase(units)) {
            return makePreeditResult(true, {}, "已加入詞庫：" + phrase_text);
        }
        return makePreeditResult(true, {}, "加入詞庫失敗");
    }

    // Any other navigation/escape exits ShiftSelecting
    state_ = InputState::DetectingZhuyin;
    if (ev.special == SpecialKey::Escape) {
        return makePreeditResult(true);
    }
    return handleKey(ev);
}

KeyResult Buffer::handleKey(const KeyEvent &ev) {
    // 1. Microsoft Bopomofo Shift/Ctrl + Punctuation (always restores DetectingZhuyin)
    if (auto punct = layout_.msPunctuation(ev)) {
        if (!pending_keys_.empty()) {
            insertRawToQueue(pending_keys_);
            pending_keys_.clear();
            max_pending_slot_ = -1;
        }
        if (cells_.empty()) {
            state_ = InputState::DetectingZhuyin;
            KeyResult res;
            res.handled = true;
            res.commit_text = *punct;
            return res;
        }
        cells_.insert(cells_.begin() + static_cast<std::ptrdiff_t>(cursor_idx_),
                      Cell{CellKind::Punctuation, *punct, ""});
        ++cursor_idx_;
        state_ = InputState::DetectingZhuyin;
        return makePreeditResult(true);
    }

    // Ctrl/Alt modified keys pass through, except Ctrl+Left/Right within active preedit/candidate
    if (ev.alt) {
        return makePreeditResult(false);
    }
    if (ev.ctrl) {
        if (state_ == InputState::CandidateOpen &&
            (ev.special == SpecialKey::Left ||
             ev.special == SpecialKey::Right)) {
            return handleCandidateKey(ev);
        }
        if (!cells_.empty() && pending_keys_.empty() &&
            (ev.special == SpecialKey::Left ||
             ev.special == SpecialKey::Right)) {
            jumpCursorByWord(ev.special == SpecialKey::Left);
            if (state_ == InputState::InQueueEnglish) {
                state_ = InputState::DetectingZhuyin;
            }
            return makePreeditResult(true);
        }
        return makePreeditResult(false);
    }

    // 2. EnglishPassthrough: direct passthrough until Space restores DetectingZhuyin
    if (state_ == InputState::EnglishPassthrough) {
        if (ev.special == SpecialKey::Space) {
            state_ = InputState::DetectingZhuyin;
        }
        return KeyResult{};
    }

    // 2b. Overlay states: CandidateOpen & ShiftSelecting
    if (state_ == InputState::CandidateOpen) {
        return handleCandidateKey(ev);
    }
    if (state_ == InputState::ShiftSelecting) {
        return handleShiftSelectKey(ev);
    }

    // 2c. Down arrow -> open candidate window, Shift+Left/Right -> enter ShiftSelecting
    if (!ev.shift && ev.special == SpecialKey::Down && tryOpenCandidates()) {
        return makePreeditResult(true);
    }
    if (ev.shift &&
        (ev.special == SpecialKey::Left || ev.special == SpecialKey::Right ||
         ev.special == SpecialKey::Home || ev.special == SpecialKey::End) &&
        !cells_.empty() && pending_keys_.empty()) {
        state_ = InputState::ShiftSelecting;
        sel_anchor_ = cursor_idx_;
        sel_focus_ = cursor_idx_;
        return handleShiftSelectKey(ev);
    }
    if (!ev.shift &&
        (ev.special == SpecialKey::Home || ev.special == SpecialKey::End) &&
        (!cells_.empty() || !pending_keys_.empty())) {
        if (pending_keys_.empty()) {
            cursor_idx_ =
                (ev.special == SpecialKey::Home) ? 0 : cells_.size();
            if (state_ == InputState::InQueueEnglish) {
                state_ = InputState::DetectingZhuyin;
            }
        }
        return makePreeditResult(true);
    }

    // 3. Shift + A..Z -> uppercase English
    if (isShiftUppercaseAlpha(ev)) {
        std::string upper(1, toUpperAscii(ev.ch));
        KeyResult res;
        flushPendingAsRaw(upper, /*enter_english_mode=*/true, res);
        return res;
    }

    // 4. InQueueEnglish: keep English/symbols in queue until Space or Backspace to Chinese
    if (state_ == InputState::InQueueEnglish) {
        if (ev.special == SpecialKey::Space) {
            insertRawToQueue(" ");
            state_ = InputState::DetectingZhuyin;
            return makePreeditResult(true);
        }
        if (ev.special == SpecialKey::Backspace) {
            if (cursor_idx_ > 0) {
                cells_.erase(cells_.begin() +
                             static_cast<std::ptrdiff_t>(cursor_idx_ - 1));
                --cursor_idx_;
            }
            updateInQueueEnglishAfterBackspace();
            return makePreeditResult(true);
        }
        if (ev.special == SpecialKey::Enter) {
            std::string commit;
            for (const auto &c : cells_) {
                commit += c.text;
            }
            reset();
            KeyResult res;
            res.handled = true;
            res.commit_text = std::move(commit);
            return res;
        }
        if (isPrintableAscii(ev)) {
            insertRawToQueue(std::string(1, ev.ch));
            return makePreeditResult(true);
        }
    }

    // 5. DetectingZhuyin
    // 5a. Backspace
    if (ev.special == SpecialKey::Backspace) {
        if (!pending_keys_.empty()) {
            pending_keys_.pop_back();
            max_pending_slot_ = -1;
            for (char k : pending_keys_) {
                max_pending_slot_ =
                    std::max(max_pending_slot_,
                             slotOrder(layout_.slotOf(KeyEvent{.ch = k})));
            }
            return makePreeditResult(true);
        }
        if (cursor_idx_ > 0) {
            cells_.erase(cells_.begin() +
                         static_cast<std::ptrdiff_t>(cursor_idx_ - 1));
            --cursor_idx_;
            return makePreeditResult(true);
        }
        return KeyResult{};
    }

    // 5b. Escape
    if (ev.special == SpecialKey::Escape) {
        if (!pending_keys_.empty()) {
            pending_keys_.clear();
            max_pending_slot_ = -1;
            return makePreeditResult(true);
        }
        if (!cells_.empty()) {
            reset();
            return makePreeditResult(true);
        }
        return KeyResult{};
    }

    // 5c. Enter
    if (ev.special == SpecialKey::Enter) {
        if (cells_.empty() && pending_keys_.empty()) {
            return KeyResult{};
        }
        std::string commit;
        for (std::size_t i = 0; i < cursor_idx_ && i < cells_.size(); ++i) {
            commit += cells_[i].text;
        }
        commit += pending_keys_;
        for (std::size_t i = cursor_idx_; i < cells_.size(); ++i) {
            commit += cells_[i].text;
        }
        reset();
        KeyResult res;
        res.handled = true;
        res.commit_text = std::move(commit);
        return res;
    }

    // 5d. Cursor Left / Right (when no pending keys)
    if (ev.special == SpecialKey::Left && pending_keys_.empty() &&
        !cells_.empty()) {
        if (cursor_idx_ > 0) {
            --cursor_idx_;
        }
        return makePreeditResult(true);
    }
    if (ev.special == SpecialKey::Right && pending_keys_.empty() &&
        !cells_.empty()) {
        if (cursor_idx_ < cells_.size()) {
            ++cursor_idx_;
        }
        return makePreeditResult(true);
    }

    // 5e. Space
    if (ev.special == SpecialKey::Space) {
        if (!pending_keys_.empty()) {
            std::string reading = pending_keys_ + ' ';
            auto cands = backend_.convertSyllable(reading);
            if (!cands.empty()) {
                cells_.insert(
                    cells_.begin() + static_cast<std::ptrdiff_t>(cursor_idx_),
                    Cell{CellKind::Chinese, cands.front().text, reading, false});
                const std::size_t inserted_idx = cursor_idx_;
                ++cursor_idx_;
                pending_keys_.clear();
                max_pending_slot_ = -1;
                resegmentAround(inserted_idx);
                return makePreeditResult(true);
            }
            KeyResult res;
            flushPendingAsRaw(" ", /*enter_english_mode=*/false, res);
            return res;
        }
        if (cells_.empty()) {
            return KeyResult{};
        }
        insertRawToQueue(" ");
        return makePreeditResult(true);
    }

    // 5f. Daqian Zhuyin slots
    Slot slot = layout_.slotOf(ev);
    if (slot == Slot::Tone) {
        if (pending_keys_.empty()) {
            // Standalone tone key without pending syllable -> treat as non-Zhuyin
            if (cells_.empty()) {
                return KeyResult{};
            }
            insertRawToQueue(std::string(1, ev.ch));
            state_ = InputState::InQueueEnglish;
            return makePreeditResult(true);
        }
        std::string reading = pending_keys_ + ev.ch;
        auto cands = backend_.convertSyllable(reading);
        if (!cands.empty()) {
            cells_.insert(
                cells_.begin() + static_cast<std::ptrdiff_t>(cursor_idx_),
                Cell{CellKind::Chinese, cands.front().text, reading, false});
            const std::size_t inserted_idx = cursor_idx_;
            ++cursor_idx_;
            pending_keys_.clear();
            max_pending_slot_ = -1;
            resegmentAround(inserted_idx);
            return makePreeditResult(true);
        }
        KeyResult res;
        flushPendingAsRaw(std::string(1, ev.ch), /*enter_english_mode=*/true,
                          res);
        return res;
    }

    if (slot == Slot::Initial || slot == Slot::Medial || slot == Slot::Final) {
        int order = slotOrder(slot);
        if (order > max_pending_slot_) {
            pending_keys_.push_back(ev.ch);
            max_pending_slot_ = order;
            return makePreeditResult(true);
        }
        // Slot conflict -> flush raw keys and enter English mode
        KeyResult res;
        flushPendingAsRaw(std::string(1, ev.ch), /*enter_english_mode=*/true,
                          res);
        return res;
    }

    // 5g. NonZhuyin printable ASCII
    if (isPrintableAscii(ev)) {
        if (!pending_keys_.empty()) {
            KeyResult res;
            flushPendingAsRaw(std::string(1, ev.ch),
                              /*enter_english_mode=*/true, res);
            return res;
        }
        if (cells_.empty()) {
            // Empty queue + non-Zhuyin (e.g. '): direct passthrough, stay DetectingZhuyin
            return KeyResult{};
        }
        insertRawToQueue(std::string(1, ev.ch));
        state_ = InputState::InQueueEnglish;
        return makePreeditResult(true);
    }

    return KeyResult{};
}

} // namespace frog_ime
