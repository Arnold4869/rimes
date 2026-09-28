#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "rime_snapshot.hpp"

namespace rimes::linuxime {

struct RimeEngineOptions {
    std::filesystem::path shared_data_dir;
    std::filesystem::path user_data_dir;
    std::filesystem::path log_dir;
    bool full_maintenance_check = false;
};

// Process-wide librime owner. One instance per Fcitx5 addon process. Sessions
// are created per input context and must not be shared across fields.
class RimeEngine final {
public:
    using SessionId = std::uintptr_t;

    RimeEngine();
    ~RimeEngine();

    RimeEngine(const RimeEngine&) = delete;
    RimeEngine& operator=(const RimeEngine&) = delete;

    bool Start(const RimeEngineOptions& options, std::string* error = nullptr) noexcept;
    void Stop() noexcept;
    [[nodiscard]] bool IsHealthy() const noexcept;

    bool RunMaintenance(bool full_check, std::string* error = nullptr) noexcept;

    SessionId CreateSession(std::string* error = nullptr) noexcept;
    bool DestroySession(SessionId session, std::string* error = nullptr) noexcept;

    bool SelectSchema(SessionId session,
                      std::string_view schema_id,
                      std::string* error = nullptr) noexcept;
    bool SetOption(SessionId session,
                   std::string_view option,
                   bool value,
                   std::string* error = nullptr) noexcept;

    // Applies one key and drains commit/context/status into output.
    bool ProcessKey(SessionId session,
                    std::int32_t keycode,
                    std::int32_t modifiers,
                    EngineSnapshot* output,
                    std::string* error = nullptr) noexcept;

    bool SelectCandidate(SessionId session,
                         int index_on_page,
                         EngineSnapshot* output,
                         std::string* error = nullptr) noexcept;

    bool ClearComposition(SessionId session,
                          EngineSnapshot* output,
                          std::string* error = nullptr) noexcept;

    bool CommitComposition(SessionId session,
                           EngineSnapshot* output,
                           std::string* error = nullptr) noexcept;

    bool Refresh(SessionId session,
                 EngineSnapshot* output,
                 std::string* error = nullptr) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace rimes::linuxime
