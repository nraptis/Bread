#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_HPP_

#include <cstring>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordInflater.hpp"

namespace bread::expansion::key_expansion {

class PasswordBallooner : public PasswordInflater {
 public:
  PasswordBallooner() {
    std::memset(mBuffer, 0, sizeof(mBuffer));
  }

  ~PasswordBallooner() override = default;

  void Inflate(unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pOutput) final {
    if (pOutput == nullptr) {
      return;
    }
    Expand(pPassword, pPasswordLength);
    std::memcpy(pOutput, mBuffer, sizeof(mBuffer));
  }

  virtual void Expand(unsigned char* pPassword,
                      int pPasswordLength) = 0;

 protected:
  unsigned char mBuffer[PASSWORD_BALLOONED_SIZE];
};

}  // namespace bread::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_HPP_
