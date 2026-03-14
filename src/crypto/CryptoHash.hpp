#ifndef INSPECT_CRYPTO_HASH_HPP_
#define INSPECT_CRYPTO_HASH_HPP_

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace inspect::crypto {

struct Sha256Context {
  std::uint32_t mState[8] = {};
  unsigned char mBuffer[64] = {};
  std::size_t mBufferLength = 0;
  std::uint64_t mBitLength = 0;
};

inline constexpr std::uint32_t kSha256InitState[8] = {
    0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
    0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U,
};

inline constexpr std::uint32_t kSha256K[64] = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U, 0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
    0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U, 0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
    0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU, 0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
    0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U, 0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
    0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U, 0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
    0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U, 0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
    0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U, 0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U, 0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U,
};

inline std::uint32_t RotR(std::uint32_t pValue, std::uint32_t pShift) {
  return (pValue >> pShift) | (pValue << (32U - pShift));
}

inline void Sha256Transform(Sha256Context& pCtx, const unsigned char pBlock[64]) {
  std::uint32_t aW[64] = {};
  for (std::size_t aIndex = 0; aIndex < 16; ++aIndex) {
    const std::size_t aBase = aIndex * 4;
    aW[aIndex] = (static_cast<std::uint32_t>(pBlock[aBase]) << 24U) |
                 (static_cast<std::uint32_t>(pBlock[aBase + 1]) << 16U) |
                 (static_cast<std::uint32_t>(pBlock[aBase + 2]) << 8U) |
                 static_cast<std::uint32_t>(pBlock[aBase + 3]);
  }
  for (std::size_t aIndex = 16; aIndex < 64; ++aIndex) {
    const std::uint32_t aS0 = RotR(aW[aIndex - 15], 7U) ^ RotR(aW[aIndex - 15], 18U) ^ (aW[aIndex - 15] >> 3U);
    const std::uint32_t aS1 = RotR(aW[aIndex - 2], 17U) ^ RotR(aW[aIndex - 2], 19U) ^ (aW[aIndex - 2] >> 10U);
    aW[aIndex] = aW[aIndex - 16] + aS0 + aW[aIndex - 7] + aS1;
  }

  std::uint32_t a = pCtx.mState[0];
  std::uint32_t b = pCtx.mState[1];
  std::uint32_t c = pCtx.mState[2];
  std::uint32_t d = pCtx.mState[3];
  std::uint32_t e = pCtx.mState[4];
  std::uint32_t f = pCtx.mState[5];
  std::uint32_t g = pCtx.mState[6];
  std::uint32_t h = pCtx.mState[7];

  for (std::size_t aRound = 0; aRound < 64; ++aRound) {
    const std::uint32_t aS1 = RotR(e, 6U) ^ RotR(e, 11U) ^ RotR(e, 25U);
    const std::uint32_t aCh = (e & f) ^ ((~e) & g);
    const std::uint32_t aTemp1 = h + aS1 + aCh + kSha256K[aRound] + aW[aRound];
    const std::uint32_t aS0 = RotR(a, 2U) ^ RotR(a, 13U) ^ RotR(a, 22U);
    const std::uint32_t aMaj = (a & b) ^ (a & c) ^ (b & c);
    const std::uint32_t aTemp2 = aS0 + aMaj;

    h = g;
    g = f;
    f = e;
    e = d + aTemp1;
    d = c;
    c = b;
    b = a;
    a = aTemp1 + aTemp2;
  }

  pCtx.mState[0] += a;
  pCtx.mState[1] += b;
  pCtx.mState[2] += c;
  pCtx.mState[3] += d;
  pCtx.mState[4] += e;
  pCtx.mState[5] += f;
  pCtx.mState[6] += g;
  pCtx.mState[7] += h;
}

inline void Sha256Init(Sha256Context& pCtx) {
  std::memcpy(pCtx.mState, kSha256InitState, sizeof(kSha256InitState));
  std::memset(pCtx.mBuffer, 0, sizeof(pCtx.mBuffer));
  pCtx.mBufferLength = 0;
  pCtx.mBitLength = 0;
}

inline void Sha256Update(Sha256Context& pCtx, const unsigned char* pData, std::size_t pLength) {
  if (pData == nullptr || pLength == 0) {
    return;
  }

  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    pCtx.mBuffer[pCtx.mBufferLength] = pData[aIndex];
    ++pCtx.mBufferLength;
    if (pCtx.mBufferLength == 64) {
      Sha256Transform(pCtx, pCtx.mBuffer);
      pCtx.mBitLength += 512U;
      pCtx.mBufferLength = 0;
    }
  }
}

inline void Sha256Final(Sha256Context& pCtx, unsigned char pDigest[32]) {
  const std::uint64_t aTotalBits = pCtx.mBitLength + (static_cast<std::uint64_t>(pCtx.mBufferLength) * 8U);

  pCtx.mBuffer[pCtx.mBufferLength] = 0x80U;
  ++pCtx.mBufferLength;

  if (pCtx.mBufferLength > 56) {
    while (pCtx.mBufferLength < 64) {
      pCtx.mBuffer[pCtx.mBufferLength++] = 0;
    }
    Sha256Transform(pCtx, pCtx.mBuffer);
    pCtx.mBufferLength = 0;
  }

  while (pCtx.mBufferLength < 56) {
    pCtx.mBuffer[pCtx.mBufferLength++] = 0;
  }
  for (std::size_t aIndex = 0; aIndex < 8; ++aIndex) {
    pCtx.mBuffer[63 - aIndex] = static_cast<unsigned char>((aTotalBits >> (8U * aIndex)) & 0xFFU);
  }
  Sha256Transform(pCtx, pCtx.mBuffer);

  for (std::size_t aWord = 0; aWord < 8; ++aWord) {
    pDigest[aWord * 4] = static_cast<unsigned char>((pCtx.mState[aWord] >> 24U) & 0xFFU);
    pDigest[aWord * 4 + 1] = static_cast<unsigned char>((pCtx.mState[aWord] >> 16U) & 0xFFU);
    pDigest[aWord * 4 + 2] = static_cast<unsigned char>((pCtx.mState[aWord] >> 8U) & 0xFFU);
    pDigest[aWord * 4 + 3] = static_cast<unsigned char>(pCtx.mState[aWord] & 0xFFU);
  }
}

inline void Sha256(const unsigned char* pData, std::size_t pLength, unsigned char pDigest[32]) {
  Sha256Context aCtx;
  Sha256Init(aCtx);
  Sha256Update(aCtx, pData, pLength);
  Sha256Final(aCtx, pDigest);
}

inline void HmacSha256(const unsigned char* pKey,
                       std::size_t pKeyLength,
                       const unsigned char* pData,
                       std::size_t pDataLength,
                       unsigned char pDigest[32]) {
  unsigned char aKeyBlock[64] = {};
  if (pKey != nullptr && pKeyLength > 0) {
    if (pKeyLength > 64) {
      Sha256(pKey, pKeyLength, aKeyBlock);
    } else {
      std::memcpy(aKeyBlock, pKey, pKeyLength);
    }
  }

  unsigned char aIpad[64] = {};
  unsigned char aOpad[64] = {};
  for (std::size_t aIndex = 0; aIndex < 64; ++aIndex) {
    aIpad[aIndex] = static_cast<unsigned char>(aKeyBlock[aIndex] ^ 0x36U);
    aOpad[aIndex] = static_cast<unsigned char>(aKeyBlock[aIndex] ^ 0x5CU);
  }

  unsigned char aInner[32] = {};
  Sha256Context aInnerCtx;
  Sha256Init(aInnerCtx);
  Sha256Update(aInnerCtx, aIpad, sizeof(aIpad));
  Sha256Update(aInnerCtx, pData, pDataLength);
  Sha256Final(aInnerCtx, aInner);

  Sha256Context aOuterCtx;
  Sha256Init(aOuterCtx);
  Sha256Update(aOuterCtx, aOpad, sizeof(aOpad));
  Sha256Update(aOuterCtx, aInner, sizeof(aInner));
  Sha256Final(aOuterCtx, pDigest);
}

inline void HkdfSha256(const unsigned char* pIkm,
                       std::size_t pIkmLength,
                       const unsigned char* pSalt,
                       std::size_t pSaltLength,
                       const unsigned char* pInfo,
                       std::size_t pInfoLength,
                       unsigned char* pOut,
                       std::size_t pOutLength) {
  if (pOut == nullptr || pOutLength == 0) {
    return;
  }

  unsigned char aZeroSalt[32] = {};
  const unsigned char* aSaltPtr = (pSalt == nullptr || pSaltLength == 0) ? aZeroSalt : pSalt;
  const std::size_t aSaltLen = (pSalt == nullptr || pSaltLength == 0) ? sizeof(aZeroSalt) : pSaltLength;

  unsigned char aPrk[32] = {};
  HmacSha256(aSaltPtr, aSaltLen, pIkm, pIkmLength, aPrk);

  unsigned char aT[32] = {};
  std::size_t aWritten = 0;
  unsigned char aCounter = 1;
  std::size_t aTLen = 0;
  while (aWritten < pOutLength) {
    Sha256Context aCtx;
    unsigned char aIpad[64] = {};
    unsigned char aOpad[64] = {};
    unsigned char aKeyBlock[64] = {};
    std::memcpy(aKeyBlock, aPrk, sizeof(aPrk));
    for (std::size_t aIndex = 0; aIndex < 64; ++aIndex) {
      aIpad[aIndex] = static_cast<unsigned char>(aKeyBlock[aIndex] ^ 0x36U);
      aOpad[aIndex] = static_cast<unsigned char>(aKeyBlock[aIndex] ^ 0x5CU);
    }

    unsigned char aInnerDigest[32] = {};
    Sha256Init(aCtx);
    Sha256Update(aCtx, aIpad, sizeof(aIpad));
    if (aTLen > 0) {
      Sha256Update(aCtx, aT, aTLen);
    }
    Sha256Update(aCtx, pInfo, pInfoLength);
    Sha256Update(aCtx, &aCounter, 1);
    Sha256Final(aCtx, aInnerDigest);

    Sha256Init(aCtx);
    Sha256Update(aCtx, aOpad, sizeof(aOpad));
    Sha256Update(aCtx, aInnerDigest, sizeof(aInnerDigest));
    Sha256Final(aCtx, aT);
    aTLen = sizeof(aT);

    const std::size_t aRemaining = pOutLength - aWritten;
    const std::size_t aChunk = aRemaining < sizeof(aT) ? aRemaining : sizeof(aT);
    std::memcpy(pOut + aWritten, aT, aChunk);
    aWritten += aChunk;
    ++aCounter;
  }
}

}  // namespace inspect::crypto

#endif  // INSPECT_CRYPTO_HASH_HPP_
