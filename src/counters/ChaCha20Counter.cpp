#include "ChaCha20Counter.hpp"

#include <cstring>

#include "CryptoHash.hpp"

namespace {

inline void QuarterRound(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d) {
  a += b;
  d ^= a;
  d = (d << 16U) | (d >> 16U);

  c += d;
  b ^= c;
  b = (b << 12U) | (b >> 20U);

  a += b;
  d ^= a;
  d = (d << 8U) | (d >> 24U);

  c += d;
  b ^= c;
  b = (b << 7U) | (b >> 25U);
}

}  // namespace

inline std::uint32_t ChaCha20Counter::ROL32(std::uint32_t v, int r) {
  return (v << r) | (v >> (32 - r));
}

inline std::uint32_t ChaCha20Counter::LoadLE32(const std::uint8_t* p) {
  return static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8U) |
         (static_cast<std::uint32_t>(p[2]) << 16U) |
         (static_cast<std::uint32_t>(p[3]) << 24U);
}

inline void ChaCha20Counter::StoreLE32(std::uint8_t* p, std::uint32_t v) {
  p[0] = static_cast<std::uint8_t>(v & 0xFFU);
  p[1] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
  p[2] = static_cast<std::uint8_t>((v >> 16U) & 0xFFU);
  p[3] = static_cast<std::uint8_t>((v >> 24U) & 0xFFU);
}

void ChaCha20Counter::SecureZero(void* p, std::size_t n) {
  volatile std::uint8_t* aBytes = static_cast<volatile std::uint8_t*>(p);
  while (n-- > 0) {
    *aBytes++ = 0;
  }
}

ChaCha20Counter::ChaCha20Counter()
    : mBlockUsed(CHACHA_BLOCK_SIZE_BYTES), mSeeded(false) {
  std::memset(mState, 0, sizeof(mState));
  std::memset(mBlock, 0, sizeof(mBlock));
}

ChaCha20Counter::~ChaCha20Counter() {
  Clear();
}

void ChaCha20Counter::Clear() {
  SecureZero(mState, sizeof(mState));
  SecureZero(mBlock, sizeof(mBlock));
  mBlockUsed = CHACHA_BLOCK_SIZE_BYTES;
  mSeeded = false;
}

bool ChaCha20Counter::SeedKeyNonce(const std::uint8_t* key32, const std::uint8_t* nonce12, std::uint32_t counter) {
  if (key32 == nullptr || nonce12 == nullptr) {
    return false;
  }

  mState[0] = 0x61707865U;
  mState[1] = 0x3320646EU;
  mState[2] = 0x79622D32U;
  mState[3] = 0x6B206574U;
  for (int aIndex = 0; aIndex < 8; ++aIndex) {
    mState[4 + aIndex] = LoadLE32(key32 + (4 * aIndex));
  }
  mState[12] = counter;
  mState[13] = LoadLE32(nonce12 + 0);
  mState[14] = LoadLE32(nonce12 + 4);
  mState[15] = LoadLE32(nonce12 + 8);

  mBlockUsed = CHACHA_BLOCK_SIZE_BYTES;
  mSeeded = true;
  return true;
}

void ChaCha20Counter::Refill() {
  std::uint32_t aWorking[16] = {};
  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    aWorking[aIndex] = mState[aIndex];
  }

  for (std::uint32_t aRound = 0; aRound < CHACHA_ROUNDS; aRound += 2U) {
    QuarterRound(aWorking[0], aWorking[4], aWorking[8], aWorking[12]);
    QuarterRound(aWorking[1], aWorking[5], aWorking[9], aWorking[13]);
    QuarterRound(aWorking[2], aWorking[6], aWorking[10], aWorking[14]);
    QuarterRound(aWorking[3], aWorking[7], aWorking[11], aWorking[15]);

    QuarterRound(aWorking[0], aWorking[5], aWorking[10], aWorking[15]);
    QuarterRound(aWorking[1], aWorking[6], aWorking[11], aWorking[12]);
    QuarterRound(aWorking[2], aWorking[7], aWorking[8], aWorking[13]);
    QuarterRound(aWorking[3], aWorking[4], aWorking[9], aWorking[14]);
  }

  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    StoreLE32(mBlock + (aIndex * 4), aWorking[aIndex] + mState[aIndex]);
  }

  ++mState[12];
  mBlockUsed = 0;
  SecureZero(aWorking, sizeof(aWorking));
}

void ChaCha20Counter::DeriveKeyNonceFromBytes(const std::uint8_t* bytes,
                                              std::size_t len,
                                              std::uint8_t outKey[CHACHA_KEY_SIZE_BYTES],
                                              std::uint8_t outNonce[CHACHA_NONCE_SIZE_BYTES]) {
  static constexpr unsigned char kSalt[] = {
      'B','r','e','a','d','-','C','h','a','C','h','a','2','0','-','s','a','l','t'
  };
  static constexpr unsigned char kInfo[] = {
      'B','r','e','a','d','-','C','h','a','C','h','a','2','0','-','k','e','y','-','n','o','n','c','e'
  };

  unsigned char aOutput[44] = {};
  inspect::crypto::HkdfSha256(bytes, len, kSalt, sizeof(kSalt), kInfo, sizeof(kInfo), aOutput, sizeof(aOutput));
  std::memcpy(outKey, aOutput, CHACHA_KEY_SIZE_BYTES);
  std::memcpy(outNonce, aOutput + CHACHA_KEY_SIZE_BYTES, CHACHA_NONCE_SIZE_BYTES);
  SecureZero(aOutput, sizeof(aOutput));
}

void ChaCha20Counter::Seed(const std::uint8_t* bytes, std::size_t len) {
  std::uint8_t aKey[CHACHA_KEY_SIZE_BYTES] = {};
  std::uint8_t aNonce[CHACHA_NONCE_SIZE_BYTES] = {};
  DeriveKeyNonceFromBytes(bytes, len, aKey, aNonce);
  SeedKeyNonce(aKey, aNonce, 0U);
  SecureZero(aKey, sizeof(aKey));
  SecureZero(aNonce, sizeof(aNonce));
}

void ChaCha20Counter::Seed(unsigned char* pPassword, int pPasswordLength) {
  if (pPassword == nullptr || pPasswordLength <= 0) {
    Seed(nullptr, 0);
    return;
  }
  Seed(reinterpret_cast<const std::uint8_t*>(pPassword), static_cast<std::size_t>(pPasswordLength));
}

std::uint32_t ChaCha20Counter::GetWord() {
  if (!mSeeded) {
    Seed(nullptr, 0);
  }
  if (mBlockUsed > CHACHA_BLOCK_SIZE_BYTES - 4U) {
    Refill();
  }
  const std::uint32_t aValue = LoadLE32(mBlock + mBlockUsed);
  mBlockUsed += 4U;
  return aValue;
}

void ChaCha20Counter::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }
  for (int aIndex = 0; aIndex < pDestinationLength; ++aIndex) {
    pDestination[aIndex] = Get();
  }
}

unsigned char ChaCha20Counter::Get() {
  if (!mSeeded) {
    Seed(nullptr, 0);
  }
  if (mBlockUsed >= CHACHA_BLOCK_SIZE_BYTES) {
    Refill();
  }
  const unsigned char aValue = mBlock[mBlockUsed];
  ++mBlockUsed;
  return aValue;
}
