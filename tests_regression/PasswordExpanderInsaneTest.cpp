#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/Tests.hpp"

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr int kDefaultTrialCount = 25;
constexpr int kDefaultBlockCount = 300;
constexpr std::size_t kWordListCount = 25U;

constexpr const char* kFruitWords[kWordListCount] = {
    "apple",      "apricot",   "banana",    "blackberry", "blueberry",
    "cherry",     "coconut",   "fig",       "grape",      "grapefruit",
    "guava",      "kiwi",      "lemon",     "lime",       "mango",
    "melon",      "nectarine", "orange",    "papaya",     "peach",
    "pear",       "pineapple", "plum",      "raspberry",  "strawberry",
};

constexpr const char* kAnimalWords[kWordListCount] = {
    "ant",       "badger",   "bear",      "beaver",   "bison",
    "cat",       "cougar",   "deer",      "dog",      "dolphin",
    "eagle",     "ferret",   "fox",       "frog",     "goat",
    "hawk",      "horse",    "koala",     "lemur",    "lion",
    "otter",     "panda",    "rabbit",    "tiger",    "wolf",
};

constexpr const char* kStyleWords[kWordListCount] = {
    "blazer",    "boots",     "cap",       "cardigan", "coat",
    "denim",     "dress",     "hoodie",    "jacket",   "jeans",
    "khakis",    "loafer",    "mittens",   "parka",    "poncho",
    "scarf",     "shirt",     "shorts",    "skirt",    "slippers",
    "sneakers",  "suit",      "sweater",   "tunic",    "vest",
};

const char* TypeName(PasswordExpander::Type pType) {
  switch (pType) {
    case PasswordExpander::kType00:
      return "ExpandPassword_00";
    case PasswordExpander::kType01:
      return "ExpandPassword_01";
    case PasswordExpander::kType02:
      return "ExpandPassword_02";
    case PasswordExpander::kType03:
      return "ExpandPassword_03";
    case PasswordExpander::kType04:
      return "ExpandPassword_04";
    case PasswordExpander::kType05:
      return "ExpandPassword_05";
    case PasswordExpander::kType06:
      return "ExpandPassword_06";
    case PasswordExpander::kType07:
      return "ExpandPassword_07";
    case PasswordExpander::kType08:
      return "ExpandPassword_08";
    case PasswordExpander::kType09:
      return "ExpandPassword_09";
    case PasswordExpander::kType10:
      return "ExpandPassword_10";
    case PasswordExpander::kType11:
      return "ExpandPassword_11";
    case PasswordExpander::kType12:
      return "ExpandPassword_12";
    case PasswordExpander::kType13:
      return "ExpandPassword_13";
    case PasswordExpander::kType14:
      return "ExpandPassword_14";
    case PasswordExpander::kType15:
      return "ExpandPassword_15";
    case PasswordExpander::kTypeCount:
      break;
  }
  return "ExpandPassword_Unknown";
}

bool ParsePositiveInt(const char* pText, int* pOutValue) {
  if (pText == nullptr || pOutValue == nullptr) {
    return false;
  }

  int aValue = 0;
  for (const char* aCursor = pText; *aCursor != '\0'; ++aCursor) {
    if (*aCursor < '0' || *aCursor > '9') {
      return false;
    }
    aValue = (aValue * 10) + (*aCursor - '0');
    if (aValue <= 0) {
      return false;
    }
  }

  *pOutValue = aValue;
  return aValue > 0;
}

std::string BuildPassword(int pTrialIndex) {
  const std::size_t aIndex = static_cast<std::size_t>(pTrialIndex) % kWordListCount;
  const std::size_t aAnimalIndex = (aIndex * 7U + 3U) % kWordListCount;
  const std::size_t aStyleIndex = (aIndex * 11U + 5U) % kWordListCount;

  switch (pTrialIndex % 8) {
    case 0:
      return "";
    case 1:
      return std::string(1, static_cast<char>('a' + static_cast<int>(aIndex % 26U)));
    case 2:
      return std::string() +
             static_cast<char>('a' + static_cast<int>(aIndex % 26U)) +
             static_cast<char>('a' + static_cast<int>(aAnimalIndex % 26U));
    case 3:
      return kFruitWords[aIndex];
    case 4:
      return kAnimalWords[aAnimalIndex];
    case 5:
      return kStyleWords[aStyleIndex];
    case 6:
      return std::string(kFruitWords[aIndex]) + " " + kAnimalWords[aAnimalIndex];
    default:
      return std::string(kFruitWords[aIndex]) + " " + kAnimalWords[aAnimalIndex] + " " + kStyleWords[aStyleIndex];
  }
}

std::uint64_t HashBytes(const unsigned char* pBytes, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBytes[aIndex]);
  }
  return aDigest;
}

std::uint64_t HashText(std::uint64_t pDigest, const char* pText) {
  if (pText == nullptr) {
    return pDigest;
  }
  for (const char* aCursor = pText; *aCursor != '\0'; ++aCursor) {
    pDigest = (pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(static_cast<unsigned char>(*aCursor));
  }
  return pDigest;
}

}  // namespace

int main(int pArgc, char** pArgv) {
  int aTrialCount = kDefaultTrialCount;
  int aBlockCount = kDefaultBlockCount;
  if (pArgc >= 2 && !ParsePositiveInt(pArgv[1], &aTrialCount)) {
    std::cerr << "[FAIL] invalid trial count\n";
    return 1;
  }
  if (pArgc >= 3 && !ParsePositiveInt(pArgv[2], &aBlockCount)) {
    std::cerr << "[FAIL] invalid block count\n";
    return 1;
  }

  const std::size_t aOutputLength =
      static_cast<std::size_t>(aBlockCount) * static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE);
  std::vector<unsigned char> aExpanded(static_cast<std::size_t>(aOutputLength), 0U);
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  std::uint64_t aDigest = 1469598103934665603ULL;
  int aAnomalyCount = 0;

  std::cout << "[INFO] password expander insane test"
            << " trials=" << aTrialCount
            << " blocks=" << aBlockCount
            << " block_bytes=" << PASSWORD_EXPANDED_SIZE
            << " types=" << PasswordExpander::kTypeCount << "\n";

  for (int aTrialIndex = 0; aTrialIndex < aTrialCount; ++aTrialIndex) {
    const std::string aPassword = BuildPassword(aTrialIndex);
    const std::vector<unsigned char> aPasswordBytes(aPassword.begin(), aPassword.end());
    unsigned char* const aPasswordPtr =
        aPasswordBytes.empty() ? nullptr : const_cast<unsigned char*>(aPasswordBytes.data());
    const std::uint64_t aPasswordDigest =
        HashBytes(aPasswordBytes.data(), aPasswordBytes.size());

    for (int aTypeIndex = 0; aTypeIndex < PasswordExpander::kTypeCount; ++aTypeIndex) {
      const PasswordExpander::Type aType = static_cast<PasswordExpander::Type>(aTypeIndex);
      PasswordExpander::ExpandPasswordBlocks(aType,
                                             aPasswordPtr,
                                             static_cast<unsigned int>(aPasswordBytes.size()),
                                             aWorker.data(),
                                             aExpanded.data(),
                                             static_cast<unsigned int>(aOutputLength));

      bool aFoundForType = false;
      for (int aLeftBlock = 0; aLeftBlock < aBlockCount; ++aLeftBlock) {
        const unsigned char* aLeft =
            aExpanded.data() + (static_cast<std::size_t>(aLeftBlock) * static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE));
        const std::uint64_t aLeftDigest = HashBytes(aLeft, PASSWORD_EXPANDED_SIZE);

        for (int aRightBlock = aLeftBlock + 1; aRightBlock < aBlockCount; ++aRightBlock) {
          const unsigned char* aRight =
              aExpanded.data() + (static_cast<std::size_t>(aRightBlock) * static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE));
          if (std::memcmp(aLeft, aRight, static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE)) != 0) {
            continue;
          }

          const int aGap = aRightBlock - aLeftBlock;
          std::cerr << "[ANOMALY] type=" << TypeName(aType)
                    << " trial=" << aTrialIndex
                    << " password=\"" << aPassword << "\""
                    << " password_len=" << aPasswordBytes.size()
                    << " password_digest=" << aPasswordDigest
                    << " block_left=" << aLeftBlock
                    << " block_right=" << aRightBlock
                    << " block_gap=" << aGap
                    << " block_digest=" << aLeftDigest << "\n";
          ++aAnomalyCount;
          aFoundForType = true;
        }
      }

      if (!aFoundForType) {
        aDigest = HashText(aDigest, TypeName(aType));
        aDigest = (aDigest * 1099511628211ULL) ^ HashBytes(aExpanded.data(), aExpanded.size());
      }
    }
  }

  if (aAnomalyCount > 0) {
    std::cerr << "[FAIL] password expander insane test detected repeated "
              << PASSWORD_EXPANDED_SIZE << "-byte blocks"
              << " anomalies=" << aAnomalyCount << "\n";
    return 1;
  }

  std::cout << "[PASS] password expander insane test passed"
            << " trials=" << aTrialCount
            << " blocks=" << aBlockCount
            << " digest=" << aDigest << "\n";
  return 0;
}
