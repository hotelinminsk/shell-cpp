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



}

#endif