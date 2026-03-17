#include "ARIA256Counter.hpp"

#include <cstring>

#include "CryptoHash.hpp"

namespace {

inline void StoreBE32(std::uint8_t* pDst, std::uint32_t pValue) {
    pDst[0] = static_cast<std::uint8_t>((pValue >> 24U) & 0xFFU);
    pDst[1] = static_cast<std::uint8_t>((pValue >> 16U) & 0xFFU);
    pDst[2] = static_cast<std::uint8_t>((pValue >> 8U) & 0xFFU);
    pDst[3] = static_cast<std::uint8_t>(pValue & 0xFFU);
}

}  // namespace

ARIA256Counter::ARIA256Counter()
    : mKey{0},
      mCounter{0},
      mBlock{0},
      mBlockUsed(64U),
      mSeeded(false) {}

ARIA256Counter::~ARIA256Counter() {
    Clear();
}

bool ARIA256Counter::SeedKeyIV(const std::uint8_t* key32, const std::uint8_t* iv16, std::uint32_t counter) {
    if (key32 == nullptr || iv16 == nullptr) {
        return false;
    }
    std::memcpy(mKey, key32, 32);
    std::memcpy(mCounter, iv16, 16);
    StoreBE32(mCounter + 12, counter);
    std::memset(mBlock, 0, sizeof(mBlock));
    mBlockUsed = 64U;
    mSeeded = true;
    return true;
}

void ARIA256Counter::Seed(const std::uint8_t* bytes, std::size_t len) {
    std::uint8_t key[32] = {};
    std::uint8_t iv[16] = {};
    DeriveKeyIVFromBytes(bytes, len, key, iv);
    SeedKeyIV(key, iv, 0U);
    SecureZero(key, sizeof(key));
    SecureZero(iv, sizeof(iv));
}

void ARIA256Counter::Seed(unsigned char* pPassword, int pPasswordLength) {
    if (pPassword == nullptr || pPasswordLength <= 0) {
        Seed(nullptr, 0);
        return;
    }
    Seed(reinterpret_cast<const std::uint8_t*>(pPassword), static_cast<std::size_t>(pPasswordLength));
}

std::uint32_t ARIA256Counter::GetWord() {
    if (!mSeeded) {
        Seed(nullptr, 0);
    }
    if (mBlockUsed > 60U) {
        Refill();
    }
    const std::uint32_t value = static_cast<std::uint32_t>(mBlock[mBlockUsed]) |
                                (static_cast<std::uint32_t>(mBlock[mBlockUsed + 1U]) << 8U) |
                                (static_cast<std::uint32_t>(mBlock[mBlockUsed + 2U]) << 16U) |
                                (static_cast<std::uint32_t>(mBlock[mBlockUsed + 3U]) << 24U);
    mBlockUsed += 4U;
    return value;
}

void ARIA256Counter::Get(unsigned char* pDestination, int pDestinationLength) {
    if (pDestination == nullptr || pDestinationLength <= 0) {
        return;
    }
    for (int i = 0; i < pDestinationLength; ++i) {
        pDestination[i] = Get();
    }
}

unsigned char ARIA256Counter::Get() {
    if (!mSeeded) {
        Seed(nullptr, 0);
    }
    if (mBlockUsed >= 64U) {
        Refill();
    }
    return mBlock[mBlockUsed++];
}

void ARIA256Counter::Clear() {
    SecureZero(mKey, sizeof(mKey));
    SecureZero(mCounter, sizeof(mCounter));
    SecureZero(mBlock, sizeof(mBlock));
    mBlockUsed = 64U;
    mSeeded = false;
}

void ARIA256Counter::SecureZero(void* p, std::size_t n) {
    volatile std::uint8_t* v = static_cast<volatile std::uint8_t*>(p);
    while (n-- > 0) {
        *v++ = 0;
    }
}

void ARIA256Counter::IncrementCounterBE(std::uint8_t pCounter[16]) {
    for (int i = 15; i >= 0; --i) {
        ++pCounter[i];
        if (pCounter[i] != 0U) {
            break;
        }
    }
}

void ARIA256Counter::DeriveKeyIVFromBytes(const std::uint8_t* bytes,
                                          std::size_t len,
                                          std::uint8_t outKey[32],
                                          std::uint8_t outIV[16]) {
    static constexpr unsigned char kSalt[] = {
        'B','r','e','a','d','-','A','R','I','A','2','5','6','-','s','a','l','t'
    };
    static constexpr unsigned char kInfo[] = {
        'B','r','e','a','d','-','A','R','I','A','2','5','6','-','k','e','y','-','i','v'
    };
    unsigned char output[48] = {};
    inspect::crypto::HkdfSha256(bytes, len, kSalt, sizeof(kSalt), kInfo, sizeof(kInfo), output, sizeof(output));
    std::memcpy(outKey, output, 32);
    std::memcpy(outIV, output + 32, 16);
    SecureZero(output, sizeof(output));
}

void ARIA256Counter::AriaLikeEncryptBlock(const std::uint8_t in[16], std::uint8_t out[16]) const {
    // Counter-mode block transform backed by SHA-256 compression over (key || block).
    // This is a deterministic ARIA-like placeholder counter core for this library stage.
    unsigned char input[48] = {};
    std::memcpy(input, mKey, 32);
    std::memcpy(input + 32, in, 16);

    unsigned char digest[32] = {};
    inspect::crypto::Sha256(input, sizeof(input), digest);
    std::memcpy(out, digest, 16);
}

void ARIA256Counter::Refill() {
    std::uint8_t tmp[16] = {};
    for (int block = 0; block < 4; ++block) {
        AriaLikeEncryptBlock(mCounter, tmp);
        std::memcpy(mBlock + (16 * block), tmp, 16);
        IncrementCounterBE(mCounter);
    }
    SecureZero(tmp, sizeof(tmp));
    mBlockUsed = 0U;
}
