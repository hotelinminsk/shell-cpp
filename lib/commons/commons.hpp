#ifndef COMMONS_H
#define COMMONS_H


#include <iostream>
#include <stdlib.h>
#include <string>
#include <cctype>
#include <algorithm>
#include <vector>
#include <unordered_map>

namespace shell_commons {
  
std::string trim(const std::string& s){
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end ? std::string(start,end) : std::string());
}

enum class COMMANDTYPES {
  BUILTIN,
  NOT_BUILTIN
};


enum class CMDS {
  EXIT = 999,
  ECHO,
  TYPE,
};



}

#endif