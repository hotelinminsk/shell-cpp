#include "commons.hpp"
#include <sys/stat.h>

namespace shell_commons {

std::string getSystemName() {
#ifdef _WIN32
    return "Windows";
#elif __linux__
    return "Linux";
#elif __APPLE__
    return "macOS";
#else
    return "Unknown";
#endif
}

std::string trim(const std::string& s){
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end   = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end ? std::string(start, end) : std::string());
}

bool directoryExists(const std::string& path){
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}




Result<RangeValue, FindError> findSubstring(const std::string& MAIN, const std::string& lookedFor)
{
    auto position = MAIN.find(lookedFor);
    if(position == std::string::npos){
        return Result<RangeValue, FindError>::Err(FindError::NillSubs); 
    }

    return Result<RangeValue, FindError>::Ok(std::make_pair(position, position + lookedFor.size() -1));
}


} // namespace shell_commons
