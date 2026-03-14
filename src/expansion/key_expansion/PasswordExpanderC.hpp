#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_C_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_C_HPP_

#include "src/expansion/key_expansion/PasswordExpander.hpp"

namespace bread::expansion::key_expansion {

class PasswordExpanderC final : public PasswordExpander {
 public:
  void Expand(unsigned char* pPassword,
              int pPasswordLength,
              unsigned char* pExpanded) override;
};

}  // namespace bread::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_C_HPP_
