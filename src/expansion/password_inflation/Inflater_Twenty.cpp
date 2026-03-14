#include "src/expansion/password_inflation/Inflater_Twenty.hpp"

namespace bread::expansion::password_inflation {

void InflaterTwenty::Inflate(unsigned char* pDestination,
                             int pDestinationLength,
                             unsigned char* pPassword,
                             int pPasswordLength,
                             bread::rng::Randomizer& pRandomizer) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }

  if (pPassword != nullptr && pPasswordLength > 0) {
    pRandomizer.Seed(pPassword, pPasswordLength);
  }

  pRandomizer.Get(pDestination, pDestinationLength);

  for (int aIndex = 0; aIndex < pDestinationLength; ++aIndex) {
    if (pPassword != nullptr && pPasswordLength > 0) {
      pDestination[aIndex] =
          static_cast<unsigned char>(pDestination[aIndex] ^ pPassword[aIndex % pPasswordLength]);
    }
  }
}

}  // namespace bread::expansion::password_inflation
