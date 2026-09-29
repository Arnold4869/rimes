#pragma once

// Future Buffer / Capsule / Mailbox integration points for the Linux IME.
//
// This file documents the hook surface. Step 1 (this directory) only implements
// the IME. Later Linux ports should attach here instead of forking key routing.
//
// 1. Commit
//    RimesIme::commitText is the only path from librime commit text into a
//    Fcitx5 InputContext (ic->commitString). A Linux Buffer would intercept
//    here the way macOS Buffer intercepts before Delivery.insert: either
//    consume the text into staged blocks or pass it through.
//
// 2. Keys
//    RimesState::keyEvent is the per-field key entry. A Buffer capture lease
//    would sit in front of process_key, matching macOS handleKeyDown gates.
//    Do not introduce a second librime session for Buffer; reuse this one.
//
// 3. Session / field isolation
//    One Rime session per Fcitx5 InputContext (RimesState). That is the
//    analogue of "one session per IMKInputController". Buffer should attach
//    as another InputContextProperty on the same IC, or as a process-wide
//    panel that borrows the focused IC's session.
//
// 4. UI
//    Candidates currently go through Fcitx5's InputPanel. A later custom
//    candidate window or Buffer inline preedit can replace
//    RimesState::updateUI without changing the engine.
//
// 5. What this frontend does not provide yet
//    - Nonactivating panels / layer-shell popups (needed for Buffer chrome)
//    - Global hotkeys while another IM is active (macOS companion LaunchAgent)
//    - Cross-batch chord pairing (macOS frontend only)
//    - IBus engine (would reuse RimeEngine + these hooks)

namespace rimes::linuxime {

inline constexpr const char* kCommitHookNote =
    "RimesIme::commitText is the single Fcitx5 commit path";

}  // namespace rimes::linuxime
