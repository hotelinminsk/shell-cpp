#include "commons.hpp"

namespace shell_commons {
    std::string getSystemName(){
    #ifdef _WIN32
        return "Windows";
    #elif __linux__
        return "Linux";
    #elif __APPLE__
        return "macOS";
    #else
        return "Unknown";
    #endif // DEBUG
}

std::string trim(const std::string& s){
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end ? std::string(start,end) : std::string());
}
}
