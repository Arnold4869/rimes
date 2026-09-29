#pragma once

#include <cstdint>
#include <string_view>

namespace rimes::linuxime {

// X11 / IBus keysyms and modifier masks that librime process_key expects.
// These match Sources/RimeBuffer/RimeKey.swift so Linux and macOS feed the
// same numeric contract into the engine.
inline constexpr std::int32_t kShiftMask = 1 << 0;
inline constexpr std::int32_t kLockMask = 1 << 1;
inline constexpr std::int32_t kControlMask = 1 << 2;
inline constexpr std::int32_t kMod1Mask = 1 << 3;
inline constexpr std::int32_t kSuperMask = 1 << 6;
inline constexpr std::int32_t kSuper2Mask = 1 << 26;
inline constexpr std::int32_t kReleaseMask = 1 << 30;

inline constexpr std::int32_t kBackspace = 0xff08;
inline constexpr std::int32_t kTab = 0xff09;
inline constexpr std::int32_t kReturn = 0xff0d;
inline constexpr std::int32_t kEscape = 0xff1b;
inline constexpr std::int32_t kHome = 0xff50;
inline constexpr std::int32_t kLeft = 0xff51;
inline constexpr std::int32_t kUp = 0xff52;
inline constexpr std::int32_t kRight = 0xff53;
inline constexpr std::int32_t kDown = 0xff54;
inline constexpr std::int32_t kPageUp = 0xff55;
inline constexpr std::int32_t kPageDown = 0xff56;
inline constexpr std::int32_t kEnd = 0xff57;
inline constexpr std::int32_t kDelete = 0xffff;
inline constexpr std::int32_t kSpace = 0x20;
inline constexpr std::int32_t kF4 = 0xffc1;

// Default product schema. Matches InputConfiguration.defaultValue on macOS.
inline constexpr const char kDefaultSchemaId[] = "rime_ice";

// Printable ASCII maps to itself. Named keys use the X11 keysyms above.
bool KeysymFromName(std::string_view name, std::int32_t* keysym) noexcept;

// Digit 1-9 (not 0) used for on-page candidate selection.
bool IsCandidateSelectDigit(std::int32_t keysym, int* index) noexcept;

}  // namespace rimes::linuxime
