#ifndef ARIA256COUNTER_HPP
#define ARIA256COUNTER_HPP

#include <cstddef>
#include <cstdint>

#include "src/rng/Counter.hpp"

class ARIA256Counter : public bread::rng::Counter {
public:
    ARIA256Counter();
    ~ARIA256Counter() override;

    bool SeedKeyIV(const std::uint8_t* key32, const std::uint8_t* iv16, std::uint32_t counter = 0U);
    void Seed(const std::uint8_t* bytes, std::size_t len);
    void Seed(unsigned char* pPassword, int pPasswordLength) override;

    std::uint32_t GetWord();
    void Get(unsigned char* pDestination, int pDestinationLength) override;
    unsigned char Get() override;

    void Clear();

private:
    static void SecureZero(void* p, std::size_t n);
    static void IncrementCounterBE(std::uint8_t pCounter[16]);
    void DeriveKeyIVFromBytes(const std::uint8_t* bytes, std::size_t len, std::uint8_t outKey[32], std::uint8_t outIV[16]);
    void AriaLikeEncryptBlock(const std::uint8_t in[16], std::uint8_t out[16]) const;
    void Refill();

private:
    std::uint8_t mKey[32];
    std::uint8_t mCounter[16];
    std::uint8_t mBlock[64];
    std::uint32_t mBlockUsed;
    bool mSeeded;
};

#endif // ARIA256COUNTER_HPP

