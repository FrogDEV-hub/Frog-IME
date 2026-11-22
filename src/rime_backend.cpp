// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright (C) 2026 Frog-IME Contributors
//
// References librime RimeApi C interface (librime, BSD-3-Clause) and
// fcitx5-rime lifecycle integration patterns (LGPL-2.1-or-later).

#include "rime_backend.h"

#include <rime_api.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace frog_ime {
namespace {

constexpr int kMaxCandidatePages = 15;

void feedKeys(RimeApi *api, RimeSessionId session_id, const std::string &keys) {
    for (char c : keys) {
        api->process_key(session_id, static_cast<unsigned char>(c), 0);
    }
}

std::vector<std::string> collectMenuCandidates(RimeApi *api,
                                               RimeSessionId session_id) {
    std::vector<std::string> result;
    for (int page = 0; page < kMaxCandidatePages; ++page) {
        RIME_STRUCT(RimeContext, ctx);
        if (!api->get_context(session_id, &ctx)) {
            break;
        }
        for (int i = 0; i < ctx.menu.num_candidates; ++i) {
            if (ctx.menu.candidates[i].text != nullptr) {
                result.emplace_back(ctx.menu.candidates[i].text);
            }
        }
        bool is_last = ctx.menu.is_last_page != 0;
        int num_cands = ctx.menu.num_candidates;
        api->free_context(&ctx);
        if (is_last || num_cands == 0) {
            break;
        }
        if (!api->change_page(session_id, False)) {
            break;
        }
    }
    return result;
}

bool selectMatchingPrefixOnPages(RimeApi *api, RimeSessionId session_id,
                                 const std::string &remaining,
                                 std::size_t &matched_len) {
    for (int page = 0; page < kMaxCandidatePages; ++page) {
        RIME_STRUCT(RimeContext, ctx);
        if (!api->get_context(session_id, &ctx)) {
            return false;
        }

        int best_idx = -1;
        std::size_t best_len = 0;
        for (int i = 0; i < ctx.menu.num_candidates; ++i) {
            if (ctx.menu.candidates[i].text == nullptr) {
                continue;
            }
            std::string text = ctx.menu.candidates[i].text;
            if (!text.empty() && remaining.rfind(text, 0) == 0 &&
                text.size() > best_len) {
                best_len = text.size();
                best_idx = i;
            }
        }

        bool is_last = ctx.menu.is_last_page != 0;
        int num_cands = ctx.menu.num_candidates;
        api->free_context(&ctx);

        if (best_idx >= 0) {
            if (api->select_candidate_on_current_page(session_id, best_idx)) {
                matched_len = best_len;
                return true;
            }
            return false;
        }
        if (is_last || num_cands == 0 || !api->change_page(session_id, False)) {
            return false;
        }
    }
    return false;
}

} // namespace

LibrimeBackend::LibrimeBackend(const std::string &user_data_dir,
                               const std::string &shared_data_dir,
                               const std::string &schema_id) {
    RimeApi *api = rime_get_api();
    if (api == nullptr) {
        return;
    }

    RIME_STRUCT(RimeTraits, traits);
    traits.shared_data_dir = shared_data_dir.c_str();
    traits.user_data_dir = user_data_dir.c_str();
    traits.distribution_name = "Frog-IME";
    traits.distribution_code_name = "frog-ime";
    traits.distribution_version = "0.1.0";
    traits.app_name = "rime.frog-ime";

    static std::once_flag setup_once;
    std::call_once(setup_once, [&]() { api->setup(&traits); });

    api->initialize(&traits);
    initialized_ = true;

    if (api->start_maintenance(False)) {
        api->join_maintenance_thread();
    }

    session_id_ = api->create_session();
    if (session_id_ != 0) {
        if (!api->select_schema(session_id_, schema_id.c_str())) {
            api->select_schema(session_id_, "bopomofo");
        }
    }
}

LibrimeBackend::~LibrimeBackend() {
    RimeApi *api = rime_get_api();
    if (api == nullptr) {
        return;
    }
    if (session_id_ != 0) {
        api->destroy_session(session_id_);
        session_id_ = 0;
    }
    if (initialized_) {
        api->finalize();
        initialized_ = false;
    }
}

std::vector<Candidate>
LibrimeBackend::convertSyllable(const std::string &keys) {
    auto strings = candidatesAt(keys);
    std::vector<Candidate> out;
    out.reserve(strings.size());
    for (auto &s : strings) {
        out.push_back(Candidate{std::move(s)});
    }
    return out;
}

std::vector<std::string>
LibrimeBackend::candidatesAt(const std::string &reading_keys) {
    if (!isReady() || reading_keys.empty()) {
        return {};
    }
    RimeApi *api = rime_get_api();
    api->clear_composition(session_id_);
    feedKeys(api, session_id_, reading_keys);
    auto candidates = collectMenuCandidates(api, session_id_);
    api->clear_composition(session_id_);
    return candidates;
}

std::string
LibrimeBackend::convertPhrase(const std::vector<std::string> &readings) {
    if (!isReady() || readings.empty()) {
        return {};
    }
    RimeApi *api = rime_get_api();
    api->clear_composition(session_id_);
    for (const auto &r : readings) {
        feedKeys(api, session_id_, r);
    }

    std::string preview;
    RIME_STRUCT(RimeContext, ctx);
    if (api->get_context(session_id_, &ctx)) {
        if (ctx.commit_text_preview != nullptr) {
            preview = ctx.commit_text_preview;
        }
        api->free_context(&ctx);
    }
    api->clear_composition(session_id_);
    return preview;
}

bool LibrimeBackend::learnPhrase(const std::vector<PhraseUnit> &units) {
    if (!isReady() || units.size() < 2) {
        return false;
    }

    std::string remaining;
    for (const auto &u : units) {
        if (u.reading_keys.empty() || u.character.empty()) {
            return false;
        }
        remaining += u.character;
    }

    RimeApi *api = rime_get_api();
    api->clear_composition(session_id_);
    for (const auto &u : units) {
        feedKeys(api, session_id_, u.reading_keys);
    }

    bool committed = false;
    const std::size_t max_steps = units.size() + 4;
    for (std::size_t step = 0; step < max_steps && !remaining.empty(); ++step) {
        std::size_t matched_len = 0;
        if (!selectMatchingPrefixOnPages(api, session_id_, remaining,
                                         matched_len)) {
            break;
        }
        remaining.erase(0, matched_len);

        RIME_STRUCT(RimeCommit, commit);
        if (api->get_commit(session_id_, &commit)) {
            committed = (commit.text != nullptr);
            api->free_commit(&commit);
            if (committed) {
                break;
            }
        }
    }

    if (remaining.empty() && !committed) {
        api->process_key(session_id_, 0xff0d, 0);
        RIME_STRUCT(RimeCommit, commit);
        if (api->get_commit(session_id_, &commit)) {
            committed = (commit.text != nullptr);
            api->free_commit(&commit);
        }
    }

    api->clear_composition(session_id_);
    return committed && remaining.empty();
}

} // namespace frog_ime
