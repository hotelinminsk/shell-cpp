#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include "../lib/commons/commons.hpp"
#include <fcntl.h>
using namespace std;
static std::vector<std::string> builtins = {"cd","type", "echo", "exit", "pwd"};

static string current_working_dir = "";

//SIGNATURES
void init_cwd();

vector<string> tokenizeString(const string& s, const char key){
  if(s.size() < 1) return {};
  string temp;
  vector<string> tokens;
  bool isStringOpened = false;
  bool in_single_quotes = false;
  bool escape_mode = false;
  char input_redir_symbol = '>';

  for(char x : s){

    if(!isStringOpened && !in_single_quotes && !escape_mode){

    }

    if(escape_mode){
      // temp += x;
      if(isStringOpened){
        if(x == '\"'){
          temp += '\"';
        }else if(x == '\\' ){
          temp += '\\';
        }else if (x == ' '){
          temp += '\\';
        }
        else{
          temp += "\\";
          temp += x;
        }

      }else{
          temp += x;
      }
      escape_mode = false;
      continue;
    }

    if(x == '\"' && !in_single_quotes && !escape_mode){
      isStringOpened = !isStringOpened;
      continue;
    }

    if(x == '\'' && !isStringOpened && !escape_mode){
      in_single_quotes = !in_single_quotes;
      continue;
    }

    if(isStringOpened && !in_single_quotes || !isStringOpened && in_single_quotes){
      if(isStringOpened && x=='\\'){
        escape_mode = true;
        continue;
      }else {
        temp += x;

      }

    }else if(!isStringOpened && !in_single_quotes){
      if(x == '\\'){
        escape_mode = true;
        continue;
      }
      if(x == key){
        if(!temp.empty()){
          tokens.push_back(temp);
          temp.clear();
        }else{
          continue;
        }
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

bool execProgram(int argc, vector<string>& args, bool hasRedir, const string& redirfile){
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
    if(hasRedir){
      int fd = open(redirfile.c_str(),O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if(fd < 0) {
        perror("open error");
        exit(1);
      }

      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    execv(path.c_str(), argv.data());
    perror("Execv failed.");
    exit(1);
  }else { int status; waitpid(pid, &status, 0); }
return true;
}

int command_CD(const string& argument){
  if(argument.empty()){

    const char* home = getenv("HOME");
    current_working_dir = home;
    chdir(home);
    return 0;
  }



  bool isRelative = true;

  if(argument[0] == '/' || argument[0] != '~'){
    isRelative = false;
  }

  if(isRelative){
    vector<std::string> tokens = tokenizeString(argument, '/');
    while(!tokens.empty()){
      //cd atmaya devam etmen lazım
      const string step = tokens.front(); // get the first
      tokens.erase(tokens.begin());

      if(!step.empty()){
        //change the directory.
        if(step == ".."){
          if(current_working_dir == getenv("HOME")){
            continue;
          }
          try{
            chdir("..");
            init_cwd(); // not really
          }catch(exception ex){
            perror(ex.what());
            exit(1);
          }

        }else if(step == "."){
          continue;
        }else if(step == "~"){
          const char* home = getenv("HOME");
          chdir(home);
          init_cwd();
        }
        else{
          const string temp = current_working_dir + "/" + step;

          if(shell_commons::directoryExists(temp)){
            try{
              chdir(step.c_str());
              init_cwd();
            }catch(exception ex){
              perror(ex.what());
              exit(1);
            }
          }
        }
      }
    }
    return 0;
  }else{
    if(shell_commons::directoryExists(argument)){
      try{
        chdir(argument.c_str());
        current_working_dir = argument;
      }catch(...){
        perror("Chdir error in command_CD");
        exit(1);
      }
      return 0;
    }else{
      cout << "cd: "<<argument<<": No such file or directory"<<endl;
      return 0;
    }
  }


}

void command_PWD(){
  init_cwd();
  cout << current_working_dir << endl;
}







bool hasSubstring(const string& mainString, const string& subs){
  if(subs.length() > mainString.length()){
    throw invalid_argument("Has substring function cant take substring longer than mainstring.");
  }

  if(mainString.empty() || subs.empty()){
    throw invalid_argument("Has substring function  cant take substring or mainstring which is empty.");
  }

  int stepcount = subs.length();
  bool is_exists = false;
  int stepper = 0;
  for(int i = 0; i<mainString.length(); i++){
    stepper = i;
    int count = 0;
    while(count < stepcount){
      if(mainString[stepper++] == subs[count++]){
        is_exists = true;
      }else {
        is_exists = false;
        break;
      }
    }
    if(!(count < stepcount) && is_exists){
      break;
    }
  }


  return is_exists;

}


int doJob(const std::string& cmd, std::vector<std::string> args,
          int& flag, int& returnvalue, std::string remainder) {

    vector<string> clean_args;
    bool has_redir = false;
    int16_t redir_fd = 1;
    string redir_filename;
    
    bool expect_filename = false;
    for(auto arg : args){
      if(expect_filename) {
        redir_filename = arg;
        has_redir = true;
        continue;
      }else{
        auto pos = arg.find('>');
        if(pos == std::string::npos){
          clean_args.push_back(arg);
          continue;
        }
        string left = arg.substr(0, pos);
        string right = arg.substr(pos + 1);

        string real_left = left;

        if(!left.empty() && left.back() == '1'){
          redir_fd = 1;
          real_left.pop_back();
        }else{
          real_left = left;
        }

        if(!real_left.empty()){
          clean_args.push_back(real_left);
        }

        string real_right = right;

        if(!right.empty()){
          redir_filename = right;
        }else{
          expect_filename = true;
        }
      }
      
    }



    
    if (cmd == "exit") {
        flag = true;
        if(!args.empty()){
          try
          {
            returnvalue = std::stoi(args[0]);
          }
          catch(const std::exception& e)
          {
            std::cerr << e.what() << '\n';
          }

        }else{
          returnvalue = 0;
        }
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

    if(cmd == "pwd"){
      command_PWD();
      return 0;
    }

    if(cmd == "cd"){
      // An absolute path starts with / and specifies a location from the root of the filesystem.
      returnvalue = command_CD(remainder);
      return 0;
    }

    // Buraya geldiysen, builtin değil
    clean_args.insert(clean_args.begin(), cmd);
    // args.insert(args.begin(), cmd);
    // execProgram(args.size(),args);
    execProgram(clean_args.size(), clean_args,has_redir, redir_filename);
    return 0;
}

void init_cwd(){
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    current_working_dir = cwd;
  } else {
    perror("getcwd() error");
    exit(1);
  }
}




int main() {
    init_cwd();
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int exitstatus = 0, exitcalled = 0;
    std::string command;


    while (!exitcalled) {
        std::cout << "$ ";
        getline(std::cin, command);
        if (command.empty()) continue;

        std::vector<std::string> tokens = tokenizeString(command, ' ');
        std::string cmd = tokens.front();
        tokens.erase(tokens.begin());



        std::string remainder;
        for(size_t i = 0; i < tokens.size(); i++){
          remainder += tokens[i];
          if(i + 1 < tokens.size()) remainder += " ";
        }
        // cout << remainder<< endl;


        doJob(cmd, tokens, exitcalled, exitstatus, remainder);
    }

    return exitstatus;
}
