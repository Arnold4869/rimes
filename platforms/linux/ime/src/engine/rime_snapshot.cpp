#include "rime_snapshot.hpp"

#include <cstring>
#include <optional>

namespace rimes::linuxime {
namespace {

struct Utf8Scalar {
    std::uint32_t value = 0;
    std::size_t width = 0;
};

std::optional<Utf8Scalar> DecodeScalar(std::string_view text, std::size_t index) noexcept {
    if (index >= text.size()) {
        return std::nullopt;
    }
    const auto first = static_cast<unsigned char>(text[index]);
    Utf8Scalar scalar;
    if (first <= 0x7fU) {
        scalar = {first, 1};
    } else if (first >= 0xc2U && first <= 0xdfU) {
        scalar = {static_cast<std::uint32_t>(first & 0x1fU), 2};
    } else if (first >= 0xe0U && first <= 0xefU) {
        scalar = {static_cast<std::uint32_t>(first & 0x0fU), 3};
    } else if (first >= 0xf0U && first <= 0xf4U) {
        scalar = {static_cast<std::uint32_t>(first & 0x07U), 4};
    } else {
        return std::nullopt;
    }
    if (scalar.width > text.size() - index) {
        return std::nullopt;
    }
    for (std::size_t offset = 1; offset < scalar.width; ++offset) {
        const auto byte = static_cast<unsigned char>(text[index + offset]);
        if ((byte & 0xc0U) != 0x80U) {
            return std::nullopt;
        }
        scalar.value = (scalar.value << 6U) | (byte & 0x3fU);
    }
    if ((scalar.width == 2 && scalar.value < 0x80U) ||
        (scalar.width == 3 && scalar.value < 0x800U) ||
        (scalar.width == 4 && scalar.value < 0x10000U) ||
        (scalar.value >= 0xd800U && scalar.value <= 0xdfffU) ||
        scalar.value > 0x10ffffU) {
        return std::nullopt;
    }
    return scalar;
}

void SetError(std::string* error, std::string_view message) noexcept {
    if (error == nullptr) {
        return;
    }
    try {
        error->assign(message);
    } catch (...) {
    }
}

}  // namespace

bool IsValidUtf8(std::string_view text) noexcept {
    std::size_t index = 0;
    while (index < text.size()) {
        const auto scalar = DecodeScalar(text, index);
        if (!scalar) {
            return false;
        }
        index += scalar->width;
    }
    return true;
}

bool CopyBoundedUtf8(const char* value,
                     std::size_t maximum_bytes,
                     std::string* output,
                     std::string* error) noexcept {
    try {
        if (output == nullptr) {
            SetError(error, "string output is null");
            return false;
        }
        if (value == nullptr) {
            output->clear();
            return true;
        }
        const void* terminator = std::memchr(value, 0, maximum_bytes + 1);
        if (terminator == nullptr) {
            SetError(error, "librime returned an unterminated or oversized string");
            return false;
        }
        const auto length =
            static_cast<std::size_t>(static_cast<const char*>(terminator) - value);
        output->assign(value, length);
        if (!IsValidUtf8(*output)) {
            SetError(error, "librime returned invalid UTF-8");
            output->clear();
            return false;
        }
        return true;
    } catch (...) {
        SetError(error, "exception while copying librime text");
        if (output != nullptr) {
            output->clear();
        }
        return false;
    }
}

}  // namespace rimes::linuxime
