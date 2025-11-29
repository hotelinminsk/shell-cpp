#ifndef COMMONS_H
#define COMMONS_H



#include <iostream>
#include <stdlib.h>
#include <string>
#include <cctype>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unistd.h>

namespace shell_commons {
  
std::string trim(const std::string& s);

std::string getSystemName();

bool directoryExists(const std::string&);


enum class COMMANDTYPES {
  BUILTIN,
  NOT_BUILTIN
};


enum class CMDS {
  EXIT = 999,
  ECHO,
  TYPE,
  PWD,
  CD
};

enum class FindError {
  NillSubs
};

template <typename T, typename E>
struct Result {
  bool ok;
  T value;
  E error;

  static Result Ok(const T& v) {
    Result r;
    r.ok = true;
    r.value = v;
    return r;
  }

  static Result Err(E e) {
    Result r;
    r.ok = false;
    r.error = e;
    return r;
  }
};

using RangeValue = std::pair<size_t, size_t>;

Result<RangeValue,FindError>findSubstring(const std::string&, const std::string&);


enum class REDIRECTTYPE {
  OVERWRITE,
  APPEND,
  NONE
};

}

#endif