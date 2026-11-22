// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "key_event.h"
#include "layout.h"
#include "rime_backend.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace frog_ime {

enum class InputState {
    DetectingZhuyin,
    EnglishPassthrough,
    InQueueEnglish,
    CandidateOpen,
    ShiftSelecting,
};

enum class CellKind {
    Chinese,
    Punctuation,
    EnglishOrSpace,
};

struct Cell {
    CellKind kind = CellKind::EnglishOrSpace;
    std::string text;
    std::string reading_keys;
    bool user_selected = false;
};

struct KeyResult {
    bool handled = false;
    std::string commit_text;
    std::string preedit;
    int cursor_bytes = 0;
    std::optional<std::pair<int, int>> selection_bytes;
    std::vector<std::string> candidates;
    int highlighted_candidate = 0;
    std::string notification;
};

class Buffer {
public:
    Buffer(const LayoutClassifier &layout, RimeBackend &backend);

    KeyResult handleKey(const KeyEvent &ev);

    [[nodiscard]] InputState state() const noexcept { return state_; }
    [[nodiscard]] const std::vector<Cell> &cells() const noexcept {
        return cells_;
    }
    [[nodiscard]] const std::string &pendingKeys() const noexcept {
        return pending_keys_;
    }

    void reset();

private:
    const LayoutClassifier &layout_;
    RimeBackend &backend_;

    InputState state_ = InputState::DetectingZhuyin;
    std::vector<Cell> cells_;
    std::size_t cursor_idx_ = 0;
    std::string pending_keys_;
    int max_pending_slot_ = -1;

    // CandidateOpen state
    std::vector<std::string> candidates_;
    int highlighted_candidate_ = 0;
    std::size_t candidate_target_idx_ = 0;
    InputState state_before_candidate_ = InputState::DetectingZhuyin;

    // ShiftSelecting state
    std::size_t sel_anchor_ = 0;
    std::size_t sel_focus_ = 0;

    [[nodiscard]] std::string pendingZhuyin() const;
    [[nodiscard]] KeyResult makePreeditResult(bool handled = true,
                                              std::string commit = {},
                                              std::string notification = {}) const;
    void insertRawToQueue(const std::string &raw);
    void flushPendingAsRaw(const std::string &extra, bool enter_english_mode,
                           KeyResult &out);
    void updateInQueueEnglishAfterBackspace();
    void resegmentAround(std::size_t idx);
    void jumpCursorByWord(bool left);
    [[nodiscard]] KeyResult handleCandidateKey(const KeyEvent &ev);
    [[nodiscard]] KeyResult handleShiftSelectKey(const KeyEvent &ev);
    [[nodiscard]] bool tryOpenCandidates();
};

} // namespace frog_ime
