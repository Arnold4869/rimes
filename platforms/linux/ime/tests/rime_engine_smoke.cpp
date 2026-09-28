#include "engine/rime_engine.hpp"
#include "engine/rime_key.hpp"
#include "engine/rime_paths.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace {

void Die(const std::string& message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TypeAscii(rimes::linuxime::RimeEngine& engine,
               rimes::linuxime::RimeEngine::SessionId session,
               std::string_view keys,
               rimes::linuxime::EngineSnapshot* last) {
    for (const char key : keys) {
        std::string error;
        if (!engine.ProcessKey(session, static_cast<std::int32_t>(key), 0, last, &error)) {
            Die("process_key failed: " + error);
        }
        if (!last->handled) {
            Die(std::string("librime did not handle key '") + key + "'");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: rimes-engine-smoke <shared-data-dir> <user-data-dir> [log-dir]\n";
        return EXIT_FAILURE;
    }

    rimes::linuxime::RimeEngineOptions options;
    options.shared_data_dir = argv[1];
    options.user_data_dir = argv[2];
    options.log_dir = argc >= 4 ? argv[3] : options.user_data_dir / "log";
    options.full_maintenance_check = true;
    options.wait_for_maintenance = false;

    std::string error;
    if (!rimes::linuxime::LooksLikeSharedData(options.shared_data_dir)) {
        Die("shared data directory has no default.yaml");
    }
    if (!rimes::linuxime::EnsureDirectory(options.user_data_dir, &error) ||
        !rimes::linuxime::EnsureDirectory(options.log_dir, &error)) {
        Die(error);
    }

    const bool first_deploy =
        !std::filesystem::exists(options.user_data_dir / "build");
    rimes::linuxime::RimeEngine engine;
    const auto start_at = std::chrono::steady_clock::now();
    if (!engine.Start(options, &error)) {
        Die("engine start failed: " + error);
    }
    const auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_at)
                              .count();
    if (first_deploy && start_ms > 3000) {
        Die("first-run Start blocked the caller for " + std::to_string(start_ms) +
            " ms");
    }
    if (engine.IsDeploying()) {
        std::cout << "ok: Start returned in " << start_ms
                  << " ms while deploying\n";
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(180);
        while (engine.IsDeploying()) {
            if (std::chrono::steady_clock::now() > deadline) {
                Die("background deploy timed out");
            }
            if (engine.PollMaintenance(&error)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    if (!engine.IsHealthy()) {
        Die("engine was not healthy after deploy: " + error);
    }
    std::cout << "ok: engine ready after async deploy (" << start_ms << " ms to return)\n";

    const auto session = engine.CreateSession(&error);
    if (session == 0) {
        Die("session create failed: " + error);
    }

    rimes::linuxime::EngineSnapshot snapshot;

    // nihao + Space => 你好
    TypeAscii(engine, session, "nihao", &snapshot);
    if (snapshot.candidates.empty()) {
        Die("nihao produced no candidates");
    }
    bool saw_nihao = false;
    for (const auto& candidate : snapshot.candidates) {
        if (candidate.text == "你好") {
            saw_nihao = true;
        }
    }
    if (!saw_nihao) {
        Die("candidate page after nihao did not include 你好");
    }
    if (!engine.ProcessKey(session, rimes::linuxime::kSpace, 0, &snapshot, &error)) {
        Die("space failed: " + error);
    }
    if (snapshot.commit_text != "你好") {
        Die("expected commit 你好, got '" + snapshot.commit_text + "'");
    }
    if (snapshot.composing || !snapshot.preedit.empty()) {
        Die("composition remained after Space commit");
    }
    std::cout << "ok: nihao + Space => 你好\n";

    // Number selection: type ni, pick candidate 2 if present.
    TypeAscii(engine, session, "ni", &snapshot);
    if (snapshot.candidates.size() < 2) {
        Die("expected at least two candidates for ni");
    }
    const std::string second = snapshot.candidates[1].text;
    if (!engine.SelectCandidate(session, 1, &snapshot, &error)) {
        Die("select candidate 2 failed: " + error);
    }
    if (snapshot.commit_text != second) {
        Die("number selection committed '" + snapshot.commit_text + "', expected '" +
            second + "'");
    }
    std::cout << "ok: number selection committed a non-first candidate\n";

    // Paging: type a syllable with many pages, page down, then up.
    TypeAscii(engine, session, "a", &snapshot);
    const int first_page = snapshot.page_no;
    const auto first_text =
        snapshot.candidates.empty() ? std::string() : snapshot.candidates.front().text;
    if (!engine.ProcessKey(session, rimes::linuxime::kPageDown, 0, &snapshot, &error)) {
        Die("page down failed: " + error);
    }
    if (snapshot.page_no <= first_page && snapshot.is_last_page &&
        snapshot.candidates.empty()) {
        Die("page down did not advance the menu");
    }
    const bool page_changed =
        snapshot.page_no != first_page ||
        (!snapshot.candidates.empty() && snapshot.candidates.front().text != first_text);
    if (!page_changed && !snapshot.is_last_page) {
        Die("page down left the first page unchanged");
    }
    if (!engine.ProcessKey(session, rimes::linuxime::kPageUp, 0, &snapshot, &error)) {
        Die("page up failed: " + error);
    }
    std::cout << "ok: paging\n";

    // Escape cancels.
    TypeAscii(engine, session, "nihao", &snapshot);
    if (!engine.ProcessKey(session, rimes::linuxime::kEscape, 0, &snapshot, &error) &&
        !engine.ClearComposition(session, &snapshot, &error)) {
        Die("escape/clear failed: " + error);
    }
    if (!snapshot.commit_text.empty()) {
        Die("escape must not commit");
    }
    if (snapshot.composing || !snapshot.preedit.empty()) {
        // Some schemas swallow Escape as handled without clearing until we ask.
        engine.ClearComposition(session, &snapshot, &error);
    }
    if (snapshot.composing || !snapshot.preedit.empty()) {
        Die("composition remained after Escape");
    }
    std::cout << "ok: Escape cancels\n";

    engine.DestroySession(session, &error);
    engine.Stop();
    std::cout << "RIMES engine smoke passed\n";
    return EXIT_SUCCESS;
}
