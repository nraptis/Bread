#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_INFLATER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_INFLATER_HPP_

namespace bread::expansion::key_expansion {

class PasswordInflater {
 public:
  virtual ~PasswordInflater() = default;

  virtual void Inflate(unsigned char* pPassword,
                       int pPasswordLength,
                       unsigned char* pOutput) = 0;
};

}  // namespace bread::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_INFLATER_HPP_
