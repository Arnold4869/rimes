#include "rime_key.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace rimes::linuxime {
namespace {

std::string LowerAscii(std::string_view name) {
    std::string lowered(name);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lowered;
}

}  // namespace

bool KeysymFromName(std::string_view name, std::int32_t* keysym) noexcept {
    if (keysym == nullptr || name.empty()) {
        return false;
    }
    if (name.size() == 1) {
        const auto ch = static_cast<unsigned char>(name[0]);
        if (ch >= 0x20 && ch <= 0x7e) {
            *keysym = static_cast<std::int32_t>(ch);
            return true;
        }
        return false;
    }

    const std::string lowered = LowerAscii(name);
    if (lowered == "space") {
        *keysym = kSpace;
    } else if (lowered == "return" || lowered == "enter") {
        *keysym = kReturn;
    } else if (lowered == "escape" || lowered == "esc") {
        *keysym = kEscape;
    } else if (lowered == "backspace") {
        *keysym = kBackspace;
    } else if (lowered == "tab") {
        *keysym = kTab;
    } else if (lowered == "page_up" || lowered == "pageup" || lowered == "prior") {
        *keysym = kPageUp;
    } else if (lowered == "page_down" || lowered == "pagedown" || lowered == "next") {
        *keysym = kPageDown;
    } else if (lowered == "left") {
        *keysym = kLeft;
    } else if (lowered == "right") {
        *keysym = kRight;
    } else if (lowered == "up") {
        *keysym = kUp;
    } else if (lowered == "down") {
        *keysym = kDown;
    } else if (lowered == "home") {
        *keysym = kHome;
    } else if (lowered == "end") {
        *keysym = kEnd;
    } else if (lowered == "delete") {
        *keysym = kDelete;
    } else if (lowered == "f4") {
        *keysym = kF4;
    } else {
        return false;
    }
    return true;
}

bool IsCandidateSelectDigit(std::int32_t keysym, int* index) noexcept {
    if (keysym < 0x31 || keysym > 0x39) {
        return false;
    }
    if (index != nullptr) {
        *index = static_cast<int>(keysym - 0x31);
    }
    return true;
}

}  // namespace rimes::linuxime
