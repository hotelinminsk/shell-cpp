#include <unistd.h>
#include <sys/wait.h>
#include "../lib/commons/commons.hpp"
using namespace std;
static std::vector<std::string> builtins = {"type", "echo", "exit"};



vector<string> tokenizeString(const string& s, const char key){
  if(s.size() < 1) return {};
  string temp; 
  vector<string> tokens;
  bool isStringOpened = false;
  for(char x : s){
    if(x == '\"'){
      
      isStringOpened = !isStringOpened;
      continue;
    }

    if(isStringOpened){
      
      temp += x;
      
    }else{
      if(x == key){
        tokens.push_back(temp);
        temp.clear();
      }
      else{
        temp += x;
      }
    }
    
  }
  if(temp.size() > 0){
    tokens.push_back(temp);
    temp.clear();
  }
  return tokens;
}

bool isBuiltin(const string& key){
  bool isfound = 0;

  for(const string& builtin :  builtins){
    if(key == builtin){
      isfound = true;break;
    }
  }


  return isfound;
}



std::string findExecutable(const std::string& name) {
    std::string path = getenv("PATH");
    if(name.find('/') != string::npos){
      return name;
    }
    
    char delimiter = (shell_commons::getSystemName() == "Windows") ? ';' : ':';
    std::vector<std::string> dirs = tokenizeString(path, delimiter);

    for (const auto& dir : dirs) {
        std::string candidate = dir + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
    }
    return "";
}

bool execProgram(int argc, vector<string>& args){
 if (args.empty()) return false;

    std::string path = findExecutable(args[0]);
    if (path.empty()) {
        std::cerr << args[0] << ": command not found" << std::endl;
        return false;
    }

  
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);


  pid_t pid = fork();
  if(pid == 0){
    execv(path.c_str(), argv.data());
    perror("Execv failed.");
    exit(1);
  }else { int status; waitpid(pid, &status, 0); }
return true;
}


int doJob(const std::string& cmd, std::vector<std::string> args,
          int& flag, int& returnvalue, std::string remainder) {

    if (cmd == "exit") {
        flag = true;
        returnvalue = std::stoi(args[0]);
        return 0;
    }

    if (cmd == "echo") {
        std::string res = shell_commons::trim(remainder);
        std::cout << res << std::endl;
        return 0;
    }

    if (cmd == "type") {
        std::string res = shell_commons::trim(remainder);
        if (isBuiltin(res))
            std::cout << res << " is a shell builtin" << std::endl;
        else {
            std::string found = findExecutable(res);
            if (!found.empty())
                std::cout << res << " is " << found << std::endl;
            else
                std::cout << res << ": not found" << std::endl;
        }
        return 0;
    }

    // Buraya geldiysen, builtin değil
    args.insert(args.begin(), cmd);
    execProgram(args.size(),args);
    return 0;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int exitstatus = 0, exitcalled = false;
    std::string command;

    while (!exitcalled) {
        std::cout << "$ ";
        getline(std::cin, command);
        if (command.empty()) continue;

        std::vector<std::string> tokens = tokenizeString(command, ' ');
        std::string cmd = tokens.front();
        tokens.erase(tokens.begin());

        std::string remainder = command.substr(cmd.size());
        doJob(cmd, tokens, exitcalled, exitstatus, remainder);
    }

    return exitstatus;
}
