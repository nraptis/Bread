#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_

#include "ByteTwister.hpp"

namespace peanutbutter::expansion::key_expansion {

class PasswordExpander final : public peanutbutter::rng::Scrambler {
 public:
  using Type = ByteTwister::Type;
  static constexpr Type kType00 = ByteTwister::kType00;
  static constexpr Type kType01 = ByteTwister::kType01;
  static constexpr Type kType02 = ByteTwister::kType02;
  static constexpr Type kType03 = ByteTwister::kType03;
  static constexpr Type kType04 = ByteTwister::kType04;
  static constexpr Type kType05 = ByteTwister::kType05;
  static constexpr Type kType06 = ByteTwister::kType06;
  static constexpr Type kType07 = ByteTwister::kType07;
  static constexpr Type kType08 = ByteTwister::kType08;
  static constexpr Type kType09 = ByteTwister::kType09;
  static constexpr Type kType10 = ByteTwister::kType10;
  static constexpr Type kType11 = ByteTwister::kType11;
  static constexpr Type kType12 = ByteTwister::kType12;
  static constexpr Type kType13 = ByteTwister::kType13;
  static constexpr Type kType14 = ByteTwister::kType14;
  static constexpr Type kType15 = ByteTwister::kType15;
  static constexpr int kTypeCount = ByteTwister::kTypeCount;

  explicit PasswordExpander(Type pType = ByteTwister::kType00)
      : mType(pType) {}

  void SetType(Type pType) { mType = pType; }
  Type CurrentType() const { return mType; }

  void Get(unsigned char* pSource,
           unsigned char* pWorker,
           unsigned char* pDestination,
           unsigned int pLength) override;

  static void FillDoubledSource(const unsigned char* pPassword,
                                unsigned int pPasswordLength,
                                unsigned char* pSourceBuffer);
  static void ExpandPassword(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pLength);
  static void ExpandPasswordBlocks(Type pType,
                                   unsigned char* pSource,
                                   unsigned int pSourceLength,
                                   unsigned char* pWorker,
                                   unsigned char* pDestination,
                                   unsigned int pOutputLength);
  static void ExpandPasswordByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pLength);
  static void ExpandPasswordBlocksByIndex(unsigned char pType,
                                          unsigned char* pSource,
                                          unsigned int pSourceLength,
                                          unsigned char* pWorker,
                                          unsigned char* pDestination,
                                          unsigned int pOutputLength);

 private:
  Type mType;
};

}  // namespace peanutbutter::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
