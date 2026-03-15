#ifndef BREAD_BREAD_HPP_
#define BREAD_BREAD_HPP_

#include <cstddef>
#include <cstdint>

#define PASSWORD_EXPANDED_SIZE 8192
#define PASSWORD_BALLOONED_SIZE (PASSWORD_EXPANDED_SIZE * 2)

namespace bread {

// !IMPORTANT: Every cipher entry point must reject any pLength that is not a
// multiple of SB_CIPHER_LENGTH_GRANULARITY.
inline constexpr std::size_t SB_CIPHER_LENGTH_GRANULARITY = 192;

inline constexpr std::size_t EB_MAX_LENGTH = 48;
inline constexpr std::size_t EB_BLOCK_SIZE_08 = 8;
inline constexpr std::size_t EB_BLOCK_SIZE_12 = 12;
inline constexpr std::size_t EB_BLOCK_SIZE_16 = 16;
inline constexpr std::size_t EB_BLOCK_SIZE_24 = 24;
inline constexpr std::size_t EB_BLOCK_SIZE_32 = 32;
inline constexpr std::size_t EB_BLOCK_SIZE_48 = 48;

inline constexpr std::size_t SB_PLAIN_TEXT_HEADER_LENGTH = 36;
inline constexpr std::size_t SB_RECOVERY_HEADER_LENGTH = 6;
inline constexpr std::size_t SB_PAYLOAD_SIZE = 250170;
inline constexpr std::size_t SB_L1_LENGTH = 250176;
inline constexpr std::size_t SB_L2_LENGTH = 500352;
inline constexpr std::size_t SB_L3_LENGTH = 1000704;

static_assert(SB_L1_LENGTH % EB_MAX_LENGTH == 0,
              "SB_L1_LENGTH must be a multiple of EB_MAX_LENGTH");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_08 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_08");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_12 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_12");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_16 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_16");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_24 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_24");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_32 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_32");
static_assert(SB_L1_LENGTH % EB_BLOCK_SIZE_48 == 0,
              "SB_L1_LENGTH must be divisible by EB_BLOCK_SIZE_48");
static_assert(SB_L1_LENGTH % SB_CIPHER_LENGTH_GRANULARITY == 0,
              "SB_L1_LENGTH must be divisible by SB_CIPHER_LENGTH_GRANULARITY");

}  // namespace bread

#endif  // BREAD_BREAD_HPP_
