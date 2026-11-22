// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright (C) 2026 Frog-IME Contributors
//
// References fcitx5 InputMethodEngineV2 / InputContextProperty /
// CommonCandidateList interfaces (fcitx5, LGPL-2.1-or-later).

#pragma once

#include "buffer.h"
#include "layout.h"
#include "rime_backend.h"

#include <fcitx/addoninstance.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include <memory>

namespace frog_ime {

class FrogImeState final : public fcitx::InputContextProperty {
public:
    FrogImeState(const LayoutClassifier &layout, RimeBackend &backend)
        : buffer(layout, backend) {}

    Buffer buffer;
};

class FrogImeEngine final : public fcitx::InputMethodEngineV2 {
public:
    explicit FrogImeEngine(fcitx::Instance *instance);

    void keyEvent(const fcitx::InputMethodEntry &entry,
                  fcitx::KeyEvent &keyEvent) override;
    void reset(const fcitx::InputMethodEntry &entry,
               fcitx::InputContextEvent &event) override;
    void deactivate(const fcitx::InputMethodEntry &entry,
                    fcitx::InputContextEvent &event) override;

private:
    fcitx::Instance *instance_;
    DaqianLayoutClassifier layout_;
    std::unique_ptr<LibrimeBackend> backend_;
    fcitx::FactoryFor<FrogImeState> factory_;

    void renderResult(fcitx::InputContext *ic, const KeyResult &res);
};

} // namespace frog_ime
