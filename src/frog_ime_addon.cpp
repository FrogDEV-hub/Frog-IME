// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright (C) 2026 Frog-IME Contributors
//
// References Fcitx5 AddonFactory registration pattern
// (fcitx5, LGPL-2.1-or-later).

#include "fcitx5_engine.h"

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>

namespace frog_ime {

class FrogImeAddonFactory final : public fcitx::AddonFactory {
public:
    fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
        return new FrogImeEngine(manager->instance());
    }
};

} // namespace frog_ime

FCITX_ADDON_FACTORY(frog_ime::FrogImeAddonFactory)
