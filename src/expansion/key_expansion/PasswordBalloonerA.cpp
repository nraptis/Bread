#include "src/expansion/key_expansion/PasswordBalloonerA.hpp"

namespace bread::expansion::key_expansion {

void PasswordBalloonerA::Expand(unsigned char* pPassword, int pPasswordLength) {
  mExpanderA.Expand(pPassword, pPasswordLength, mBuffer);
  mExpanderB.Expand(pPassword, pPasswordLength, mBuffer + PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
