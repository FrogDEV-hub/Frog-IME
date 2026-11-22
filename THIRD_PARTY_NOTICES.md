# Third-Party Notices and Attributions

`Frog-IME` is an original, clean-room implementation licensed under **LGPL-2.1-or-later** (see [LICENSE](LICENSE)).

In accordance with our open-source architecture and attribution policy (see `docs/adr/0001-clean-room-fcitx5-lgpl-with-abstraction.md`), this project references the public interfaces, addon conventions, and architectural patterns of the following upstream open-source projects:

1. **Fcitx5** (`fcitx5`)
   - Upstream: https://github.com/fcitx/fcitx5
   - License: LGPL-2.1-or-later
   - Referenced elements: `fcitx::InputMethodEngineV2`, `fcitx::AddonFactory`, `fcitx::InputContextProperty`, `fcitx::CandidateWord`, `fcitx::CommonCandidateList`, and Fcitx5 CMake / `.conf` addon registration conventions.

2. **Fcitx5 Rime** (`fcitx5-rime`)
   - Upstream: https://github.com/fcitx/fcitx5-rime
   - License: LGPL-2.1-or-later
   - Referenced elements: `librime` lifecycle integration patterns within Fcitx5 (`RimeApi` initialization, session management, schema/deployer setup).

3. **Rime Input Method Engine** (`librime`)
   - Upstream: https://github.com/rime/librime
   - License: BSD-3-Clause
   - Referenced elements: `rime_api.h` (`RimeApi`, `RimeTraits`, `RimeContext`, `RimeCommit`, `RimeSessionId`) and `bopomofo` / `bopomofo_tw` schema conventions.

4. **Catch2** (test dependency)
   - Upstream: https://github.com/catchorg/Catch2
   - License: BSL-1.0
   - Usage: Unit and integration test framework.
