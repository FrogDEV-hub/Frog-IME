// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright (C) 2026 Frog-IME Contributors
//
// References librime RimeApi C interface (librime, BSD-3-Clause) and
// fcitx5-rime lifecycle integration patterns (LGPL-2.1-or-later).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace frog_ime {

struct Candidate {
    std::string text;
};

struct PhraseUnit {
    std::string reading_keys;
    std::string character;
};

class RimeBackend {
public:
    virtual ~RimeBackend() = default;

    [[nodiscard]] virtual std::vector<Candidate>
    convertSyllable(const std::string &keys) = 0;

    [[nodiscard]] virtual std::vector<std::string>
    candidatesAt(const std::string &reading_keys) = 0;

    [[nodiscard]] virtual std::string
    convertPhrase(const std::vector<std::string> &readings) = 0;

    virtual bool learnPhrase(const std::vector<PhraseUnit> &units) = 0;
};

class LibrimeBackend final : public RimeBackend {
public:
    explicit LibrimeBackend(
        const std::string &user_data_dir,
        const std::string &shared_data_dir = "/usr/share/rime-data",
        const std::string &schema_id = "bopomofo_tw");
    ~LibrimeBackend() override;

    LibrimeBackend(const LibrimeBackend &) = delete;
    LibrimeBackend &operator=(const LibrimeBackend &) = delete;

    [[nodiscard]] bool isReady() const noexcept { return session_id_ != 0; }

    [[nodiscard]] std::vector<Candidate>
    convertSyllable(const std::string &keys) override;

    [[nodiscard]] std::vector<std::string>
    candidatesAt(const std::string &reading_keys) override;

    [[nodiscard]] std::string
    convertPhrase(const std::vector<std::string> &readings) override;

    bool learnPhrase(const std::vector<PhraseUnit> &units) override;

private:
    uintptr_t session_id_ = 0;
    bool initialized_ = false;
};

} // namespace frog_ime
