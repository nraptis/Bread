#ifndef BREAD_SRC_EXPANSION_PASSWORD_INFLATION_PASSWORD_INFLATER_HPP_
#define BREAD_SRC_EXPANSION_PASSWORD_INFLATION_PASSWORD_INFLATER_HPP_

#include "src/rng/Randomizer.hpp"

namespace bread::expansion::password_inflation {

class PasswordInflater {
 public:
  virtual ~PasswordInflater() = default;

  virtual void Inflate(unsigned char* pDestination,
                       int pDestinationLength,
                       unsigned char* pPassword,
                       int pPasswordLength,
                       bread::rng::Randomizer& pRandomizer) = 0;
};

}  // namespace bread::expansion::password_inflation

#endif  // BREAD_SRC_EXPANSION_PASSWORD_INFLATION_PASSWORD_INFLATER_HPP_

