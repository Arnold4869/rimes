#include "engine/rime_snapshot.hpp"

#include <cstdlib>
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

    Check(IsValidUtf8("nihao"), "ascii is valid");
    Check(IsValidUtf8("你好"), "CJK is valid");
    Check(IsValidUtf8(""), "empty is valid");
    Check(!IsValidUtf8(std::string_view("\xff", 1)), "0xFF is invalid");
    Check(!IsValidUtf8(std::string_view("\xed\xa0\x80", 3)), "surrogate is invalid");

    std::string copied;
    std::string error;
    Check(CopyBoundedUtf8("你好", kMaxTextBytes, &copied, &error) && copied == "你好",
          "copy CJK");
    Check(CopyBoundedUtf8(nullptr, kMaxTextBytes, &copied, &error) && copied.empty(),
          "null becomes empty");
    Check(!CopyBoundedUtf8("\xff", kMaxTextBytes, &copied, &error),
          "invalid UTF-8 is rejected");

    if (failures != 0) {
        std::cerr << failures << " snapshot test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "RIMES snapshot tests passed\n";
    return EXIT_SUCCESS;
}
