// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rime_backend.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct TempDir {
    fs::path path;
    explicit TempDir(const std::string &name)
        : path(fs::temp_directory_path() / name) {
        fs::remove_all(path);
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

} // namespace

TEST_CASE("LibrimeBackend converts Bopomofo syllables and learns phrases",
          "[rime][integration]") {
    TempDir tmp("frog_ime_rime_test_dir");
    frog_ime::LibrimeBackend backend(tmp.path.string());

    REQUIRE(backend.isReady());

    SECTION("convertSyllable returns Traditional Chinese candidates for su3") {
        auto candidates = backend.convertSyllable("su3");
        REQUIRE_FALSE(candidates.empty());
        bool has_ni = std::any_of(
            candidates.begin(), candidates.end(),
            [](const frog_ime::Candidate &c) { return c.text == "你"; });
        REQUIRE(has_ni);
    }

    SECTION("convertSyllable supports Tone-1 with trailing space (e.g. 5u )") {
        auto candidates = backend.convertSyllable("5u ");
        REQUIRE_FALSE(candidates.empty());
    }

    SECTION("candidatesAt returns candidate strings for a reading") {
        auto cands = backend.candidatesAt("cl3");
        REQUIRE_FALSE(cands.empty());
        bool has_hao = std::find(cands.begin(), cands.end(), "好") != cands.end();
        REQUIRE(has_hao);
    }

    SECTION("convertPhrase returns full sentence/phrase preview for multiple syllables") {
        auto phrase = backend.convertPhrase({"j4", "xu3"});
        REQUIRE(phrase == "物理");
    }

    SECTION("learnPhrase replays readings and commits phrase to userdb") {
        // Reject single-character or empty learnPhrase
        REQUIRE_FALSE(backend.learnPhrase({}));
        REQUIRE_FALSE(backend.learnPhrase({{"su3", "你"}}));

        // Learn a 2-character phrase and verify it succeeds
        std::vector<frog_ime::PhraseUnit> units = {
            {"su3", "你"},
            {"cl3", "好"},
        };
        REQUIRE(backend.learnPhrase(units));

        // After learning, querying combined reading should yield "你好"
        auto combined = backend.candidatesAt("su3cl3");
        REQUIRE_FALSE(combined.empty());
        REQUIRE(combined.front() == "你好");
    }
}
