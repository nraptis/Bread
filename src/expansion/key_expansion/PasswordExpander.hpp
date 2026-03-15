#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordInflater.hpp"

namespace bread::expansion::key_expansion {

class PasswordExpander : public PasswordInflater {
 public:
  PasswordExpander() {
    std::memset(mBuffer, 0, sizeof(mBuffer));
    std::memset(mWorkspace, 0, sizeof(mWorkspace));
  }

  virtual ~PasswordExpander() = default;

  void Inflate(unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pOutput) final {
    Expand(pPassword, pPasswordLength, pOutput);
  }

  virtual void Expand(unsigned char* pPassword,
                      int pPasswordLength,
                      unsigned char* pExpanded) = 0;

 protected:
  static constexpr bool IsExpandedSizeValid() {
    return PASSWORD_EXPANDED_SIZE > 0 &&
           ((PASSWORD_EXPANDED_SIZE & (PASSWORD_EXPANDED_SIZE - 1)) == 0);
  }

  static void CrashIfExpandedSizeInvalid(const char* pExpanderName) {
    if (IsExpandedSizeValid()) {
      return;
    }

    std::fprintf(stderr,
                 "[FATAL] %s requires PASSWORD_EXPANDED_SIZE to be a positive power of two for '& kMask' wrapping; got %d\n",
                 (pExpanderName != nullptr) ? pExpanderName : "PasswordExpander",
                 PASSWORD_EXPANDED_SIZE);
    std::fflush(stderr);
    std::abort();
  }

  void FillBufferFromPassword(unsigned char* pPassword, int pPasswordLength) {
    if (pPassword != nullptr && pPasswordLength > 0) {
      for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
        mBuffer[aIndex] = pPassword[aIndex % pPasswordLength];
      }
    } else {
      std::memset(mBuffer, 0, sizeof(mBuffer));
    }
  }

  template <typename TCounter>
  void PrimeWorkspace(unsigned char* pPassword, int pPasswordLength) {
    FillBufferFromPassword(pPassword, pPasswordLength);
    TCounter aCounter;
    aCounter.Seed(mBuffer, PASSWORD_EXPANDED_SIZE);
    aCounter.Get(mWorkspace, PASSWORD_EXPANDED_SIZE);
  }

  unsigned char mBuffer[PASSWORD_EXPANDED_SIZE];
  unsigned char mWorkspace[PASSWORD_EXPANDED_SIZE];
};

}  // namespace bread::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
