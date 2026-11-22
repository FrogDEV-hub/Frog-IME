// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright (C) 2026 Frog-IME Contributors
//
// References fcitx5 InputMethodEngineV2, CommonCandidateList, and
// fcitx5-rime user directory patterns (LGPL-2.1-or-later).

#include "fcitx5_engine.h"

#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/text.h>

#include <filesystem>
#include <string>

namespace frog_ime {
namespace {

class FrogCandidateWord final : public fcitx::CandidateWord {
public:
    explicit FrogCandidateWord(std::string text)
        : fcitx::CandidateWord(fcitx::Text(std::move(text))) {}

    void select(fcitx::InputContext * /*inputContext*/) const override {}
};

std::string resolveRimeUserDir() {
    std::filesystem::path dir =
        std::filesystem::path(fcitx::StandardPath::global().userDirectory(
            fcitx::StandardPath::Type::PkgData)) /
        "frog-ime" / "rime";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.string();
}

KeyEvent translateKey(const fcitx::Key &key) {
    KeyEvent ev;
    const auto states = key.states();
    ev.shift = states.test(fcitx::KeyState::Shift);
    ev.ctrl = states.test(fcitx::KeyState::Ctrl);
    ev.alt = states.test(fcitx::KeyState::Alt);

    const auto sym = key.sym();
    switch (sym) {
    case FcitxKey_space:
        ev.special = SpecialKey::Space;
        ev.ch = ' ';
        return ev;
    case FcitxKey_Return:
    case FcitxKey_KP_Enter:
        ev.special = SpecialKey::Enter;
        return ev;
    case FcitxKey_BackSpace:
        ev.special = SpecialKey::Backspace;
        return ev;
    case FcitxKey_Escape:
        ev.special = SpecialKey::Escape;
        return ev;
    case FcitxKey_Tab:
        ev.special = SpecialKey::Tab;
        return ev;
    case FcitxKey_Left:
        ev.special = SpecialKey::Left;
        return ev;
    case FcitxKey_Right:
        ev.special = SpecialKey::Right;
        return ev;
    case FcitxKey_Up:
        ev.special = SpecialKey::Up;
        return ev;
    case FcitxKey_Down:
        ev.special = SpecialKey::Down;
        return ev;
    case FcitxKey_Home:
        ev.special = SpecialKey::Home;
        return ev;
    case FcitxKey_End:
        ev.special = SpecialKey::End;
        return ev;
    case FcitxKey_Page_Up:
        ev.special = SpecialKey::PageUp;
        return ev;
    case FcitxKey_Page_Down:
        ev.special = SpecialKey::PageDown;
        return ev;
    default:
        break;
    }

    if (sym >= FcitxKey_KP_0 && sym <= FcitxKey_KP_9) {
        ev.ch = static_cast<char>('0' + (sym - FcitxKey_KP_0));
        return ev;
    }
    if (sym >= 0x20 && sym <= 0x7e) {
        ev.ch = static_cast<char>(sym);
    }
    return ev;
}

} // namespace

FrogImeEngine::FrogImeEngine(fcitx::Instance *instance)
    : instance_(instance),
      backend_(std::make_unique<LibrimeBackend>(resolveRimeUserDir())),
      factory_([this](fcitx::InputContext &) {
          return new FrogImeState(layout_, *backend_);
      }) {
    instance_->inputContextManager().registerProperty("frog-ime-state",
                                                      &factory_);
}

void FrogImeEngine::keyEvent(const fcitx::InputMethodEntry & /*entry*/,
                             fcitx::KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return;
    }

    auto *ic = keyEvent.inputContext();
    auto *state = ic->propertyFor(&factory_);
    KeyEvent ev = translateKey(keyEvent.key());
    KeyResult res = state->buffer.handleKey(ev);

    if (!res.handled) {
        return;
    }

    keyEvent.filterAndAccept();
    if (!res.commit_text.empty()) {
        ic->commitString(res.commit_text);
    }
    renderResult(ic, res);
}

void FrogImeEngine::reset(const fcitx::InputMethodEntry & /*entry*/,
                          fcitx::InputContextEvent &event) {
    auto *ic = event.inputContext();
    auto *state = ic->propertyFor(&factory_);
    state->buffer.reset();
    ic->inputPanel().reset();
    ic->updatePreedit();
    ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

void FrogImeEngine::deactivate(const fcitx::InputMethodEntry &entry,
                               fcitx::InputContextEvent &event) {
    reset(entry, event);
}

void FrogImeEngine::renderResult(fcitx::InputContext *ic,
                                 const KeyResult &res) {
    auto &panel = ic->inputPanel();
    panel.reset();

    if (!res.preedit.empty()) {
        fcitx::Text preedit;
        if (res.selection_bytes.has_value()) {
            const auto [s_raw, e_raw] = *res.selection_bytes;
            const std::size_t n = res.preedit.size();
            const std::size_t s =
                std::min(static_cast<std::size_t>(std::max(0, s_raw)), n);
            const std::size_t e =
                std::min(static_cast<std::size_t>(std::max(0, e_raw)), n);
            if (s > 0) {
                preedit.append(res.preedit.substr(0, s),
                               fcitx::TextFormatFlag::Underline);
            }
            if (e > s) {
                preedit.append(res.preedit.substr(s, e - s),
                               fcitx::TextFormatFlag::HighLight);
            }
            if (e < n) {
                preedit.append(res.preedit.substr(e),
                               fcitx::TextFormatFlag::Underline);
            }
        } else {
            preedit.append(res.preedit, fcitx::TextFormatFlag::Underline);
        }
        preedit.setCursor(res.cursor_bytes);
        panel.setClientPreedit(preedit);
    }

    if (!res.candidates.empty()) {
        auto list = std::make_unique<fcitx::CommonCandidateList>();
        list->setPageSize(9);
        list->setSelectionKey(
            fcitx::Key::keyListFromString("1 2 3 4 5 6 7 8 9"));
        for (const auto &c : res.candidates) {
            list->append<FrogCandidateWord>(c);
        }
        list->setPage(res.highlighted_candidate / 9);
        list->setGlobalCursorIndex(res.highlighted_candidate);
        panel.setCandidateList(std::move(list));
    }

    if (!res.notification.empty()) {
        panel.setAuxDown(fcitx::Text(res.notification));
    }

    ic->updatePreedit();
    ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

} // namespace frog_ime
