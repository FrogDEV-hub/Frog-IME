// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "key_event.h"

#include <optional>
#include <string>

namespace frog_ime {

enum class Slot {
    Initial,
    Medial,
    Final,
    Tone,
    NonZhuyin,
};

class LayoutClassifier {
public:
    virtual ~LayoutClassifier() = default;

    [[nodiscard]] virtual Slot slotOf(const KeyEvent &ev) const = 0;
    [[nodiscard]] virtual std::string toZhuyinSymbol(char key) const = 0;
    [[nodiscard]] virtual std::optional<std::string>
    msPunctuation(const KeyEvent &ev) const = 0;
};

class DaqianLayoutClassifier final : public LayoutClassifier {
public:
    [[nodiscard]] Slot slotOf(const KeyEvent &ev) const override;
    [[nodiscard]] std::string toZhuyinSymbol(char key) const override;
    [[nodiscard]] std::optional<std::string>
    msPunctuation(const KeyEvent &ev) const override;
};

} // namespace frog_ime
