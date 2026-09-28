#include "engine/rime_key.hpp"

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

    std::int32_t keysym = 0;
    Check(KeysymFromName("n", &keysym) && keysym == 'n', "letter n");
    Check(KeysymFromName(" ", &keysym) && keysym == kSpace, "space character");
    Check(KeysymFromName("space", &keysym) && keysym == kSpace, "space name");
    Check(KeysymFromName("Escape", &keysym) && keysym == kEscape, "escape");
    Check(KeysymFromName("Page_Down", &keysym) && keysym == kPageDown, "page down");
    Check(KeysymFromName("F4", &keysym) && keysym == kF4, "F4");
    Check(!KeysymFromName("", &keysym), "empty name");
    Check(!KeysymFromName("not-a-key", &keysym), "unknown name");

    int index = -1;
    Check(IsCandidateSelectDigit('1', &index) && index == 0, "digit 1");
    Check(IsCandidateSelectDigit('9', &index) && index == 8, "digit 9");
    Check(!IsCandidateSelectDigit('0', &index), "digit 0 is not a select key");
    Check(!IsCandidateSelectDigit('a', &index), "letter is not a select key");

    if (failures != 0) {
        std::cerr << failures << " key test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "RIMES key tests passed\n";
    return EXIT_SUCCESS;
}
