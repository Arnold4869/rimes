#include "engine/rime_paths.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    using namespace rimes::linuxime;

    const auto tmp = std::filesystem::temp_directory_path() / "rimes-paths-test";
    std::filesystem::remove_all(tmp);
    std::string error;
    Check(EnsureDirectory(tmp / "user", &error), "create user dir");
    Check(std::filesystem::is_directory(tmp / "user"), "user dir exists");
    Check(!EnsureDirectory("relative", &error), "relative path is rejected");

    setenv("RIMES_SHARED_DIR", "/tmp/rimes-shared", 1);
    setenv("RIMES_USER_DIR", "/tmp/rimes-user", 1);
    setenv("RIMES_LOG_DIR", "/tmp/rimes-log", 1);
    const auto paths = ResolveEnginePaths();
    Check(paths.shared_data_dir == "/tmp/rimes-shared", "shared override");
    Check(paths.user_data_dir == "/tmp/rimes-user", "user override");
    Check(paths.log_dir == "/tmp/rimes-log", "log override");
    Check(paths.user_data_dir.filename() != "rime" ||
              paths.user_data_dir.parent_path().filename() != "fcitx5",
          "user dir is not the stock fcitx5-rime path");

    std::filesystem::remove_all(tmp);
    if (failures != 0) {
        std::cerr << failures << " path test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "RIMES path tests passed\n";
    return EXIT_SUCCESS;
}
