#ifndef BREAD_SRC_EXPANSION_PASSWORD_INFLATION_INFLATER_TWENTY_HPP_
#define BREAD_SRC_EXPANSION_PASSWORD_INFLATION_INFLATER_TWENTY_HPP_

#include "src/expansion/password_inflation/PasswordInflater.hpp"

namespace bread::expansion::password_inflation {

class InflaterTwenty final : public PasswordInflater {
 public:
  void Inflate(unsigned char* pDestination,
               int pDestinationLength,
               unsigned char* pPassword,
               int pPasswordLength,
               bread::rng::Randomizer& pRandomizer) override;
};

}  // namespace bread::expansion::password_inflation

#endif  // BREAD_SRC_EXPANSION_PASSWORD_INFLATION_INFLATER_TWENTY_HPP_

