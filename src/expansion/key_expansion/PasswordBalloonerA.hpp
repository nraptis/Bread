#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_A_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_A_HPP_

#include "src/expansion/key_expansion/PasswordBallooner.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"

namespace bread::expansion::key_expansion {

class PasswordBalloonerA final : public PasswordBallooner {
 public:
  void Expand(unsigned char* pPassword, int pPasswordLength) override;

 private:
  PasswordExpanderA mExpanderA;
  PasswordExpanderA mExpanderB;
};

}  // namespace bread::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_BALLOONER_A_HPP_
