#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include "../lib/commons/commons.hpp"
#include <fcntl.h>
#include <unordered_set>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
using namespace std;

static std::vector<std::string> builtins = {"cd","type", "echo", "exit", "pwd"};

static vector<string> path_executables;
static vector<string> full_paths_of_execs;
static vector<string> completion_matches;


static string current_working_dir = "";

//SIGNATURES
void init_cwd();

static void build_completion_matches(const char* text);
void build_path_exec_cache();
char* command_generator(const char* text, int state);
char** my_completion(const char* text, int start, int end);

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

char** findExecutable(){
  static size_t len = 0;
  const char* PATH = getenv("PATH");
  
  char** executables = nullptr;

  char delimiter = (shell_commons::getSystemName() == "Windows") ? ';' : ':';
  
  vector<string> dirs = tokenizeString(PATH, delimiter);
  DIR* directorystream;
  struct dirent* direntry;
  int count = 0;
  for(auto dir : dirs){
    if((directorystream = opendir(dir.c_str())) == NULL){
      cout << "Could not open directory : "<< dir << endl;
      exit(-1);
    }

    while((direntry = readdir(directorystream)) != NULL){
      if(direntry->d_name == "." || direntry->d_name==".."){
        continue;
      }
      string candidate = dir + "/"+direntry->d_name;
      if(access(candidate.c_str(), X_OK) == 0 ){
        // we are sure that this is an executable.
        // the executable is : direntry->d_name
        // execs path is : dir/direntry->d_name
        executables = (char**)realloc(executables, (count + 2)* sizeof(char*));
        
        executables[count++] = strdup(direntry->d_name);
        executables[count] = nullptr;
      }
    }
  }

  return executables;
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

bool execProgram(int argc, vector<string>& args, bool hasRedir, const string& redirfile, const int redirfd, const shell_commons::REDIRECTTYPE RTYPE){
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
      int flags = O_WRONLY | O_CREAT;
      if(RTYPE == shell_commons::REDIRECTTYPE::APPEND){
        flags |= O_APPEND;
      }else{
        flags |= O_TRUNC;
      }

      int fd = open(redirfile.c_str(),flags, 0644);
      if(fd < 0) {
        perror("open error");
        exit(1);
      }

      dup2(fd, redirfd);
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
    shell_commons::REDIRECTTYPE RTYPE = shell_commons::REDIRECTTYPE::NONE;

    bool expect_filename = false;
    for(auto arg : args){
      if(expect_filename) {
        redir_filename = arg;
        has_redir = true;
        expect_filename = false;
        continue;
      }else{
        bool is_append = false;
        auto pos_append = arg.find(">>");
        auto pos_over = arg.find(">");
        size_t pos = string::npos;

        if(pos_append != string::npos){
          is_append = true;
          RTYPE = shell_commons::REDIRECTTYPE::APPEND;
          pos = pos_append;
        }else if(pos_over != string::npos){
          RTYPE = shell_commons::REDIRECTTYPE::OVERWRITE;
          pos = pos_over;
        }

        if(pos == std::string::npos){
          clean_args.push_back(arg);
          continue;
        }
        string left, right;
        if(is_append){
          left = arg.substr(0, pos);
          right = arg.substr(pos+2);
        }else{
          left = arg.substr(0,pos);
          right = arg.substr(pos+1);
        }

        string real_left = left;

        if(!left.empty()){
          if(left.back() == '1'){
            redir_fd = 1;
            real_left.pop_back();
          }else if(left.back() == '2'){
            redir_fd = 2;
            real_left.pop_back();
          }
        }else{
          real_left = left;
        }

        if(!real_left.empty()){
          clean_args.push_back(real_left);
        }

        string real_right = right;

        if(!right.empty()){
          redir_filename = right;
          has_redir = true;
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
      int old_stdout = dup(STDOUT_FILENO);
      string res = shell_commons::trim(remainder);
        if(has_redir){
          
          int flags = O_WRONLY | O_CREAT;
          if(RTYPE == shell_commons::REDIRECTTYPE::APPEND){
            flags |= O_APPEND;
          }else{
            flags |= O_TRUNC;
          }
          int fd = open(redir_filename.c_str(),flags, 0644);
          if(fd < 1){
            perror("Open error on echo.");
            exit(1);
          }
          dup2(fd, redir_fd);
          close(fd);
          while(!clean_args.empty()){
            cout << clean_args.back();
            clean_args.pop_back();
          }
          cout << endl;
        }else{
          std::cout << res << std::endl;
        }
        

      
        fflush(stdout);
        dup2(old_stdout, STDOUT_FILENO);      // 4) stdout'u eski haline getir (terminal)
        close(old_stdout);  
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
    execProgram(clean_args.size(), clean_args,has_redir, redir_filename,redir_fd,RTYPE);
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
    build_path_exec_cache();
    rl_bind_key('\t', rl_complete);
    rl_attempted_completion_function = my_completion;
    rl_completion_append_character = ' ';
    
    // while(true){
    //   char* line = readline("mysh> ");

    //   if(line == nullptr){
    //     break;
    //   }

    //   if(*line){
    //     add_history(line);
    //   }

    //   write(1, line, strlen(line));

    //   break;

    //   free(line);

    // }
    // return 0; 
    init_cwd();
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int exitstatus = 0, exitcalled = 0;
    std::string command;
    char* line;

    while (!exitcalled) {

        line = readline("$ ");
        if (line == nullptr) break;; // EOF (Ctrl+D)
        if (*line) {
            add_history(line);
        }
        // std::cout << "$ ";
        // getline(std::cin, command);
        // if (command.empty()) continue;
        command = std::string(line);
        free(line);        
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




char* command_generator(const char* text, int state){
  if (state == 0){
    build_completion_matches(text);
  }

  if(static_cast<size_t>(state) >= completion_matches.size()) return nullptr;


  const string& candidate = completion_matches[state];

  return strdup(candidate.c_str());
}

char** my_completion(const char* text, int start, int end){
  if(start != 0){
    return nullptr;
  }

  rl_attempted_completion_over = 1;

  return rl_completion_matches(text, command_generator);
}



static void build_completion_matches(const char* text){
  completion_matches.clear();

  size_t len = strlen(text);

  // first builtins 

  for(const auto& b: builtins){
    if(b.compare(0,len, text) == 0){
      completion_matches.push_back(b);
    }
  }


  // execs
  
  // for(int i = 0 ; i < path_executables.size(); i++){
  //   const auto& cmd = path_executables[i];
  //   if(cmd.compare(0,len,text) == 0){
  //     completion_matches.push_back(full_paths_of_execs[i]);
  //   }
  // }
   for(const auto& cmd: path_executables){
     if(cmd.compare(0, len, text) == 0){
       completion_matches.push_back(cmd);
    }
  }
}


void build_path_exec_cache(){
  if(!path_executables.empty()) return;

  const char* PATH = getenv("PATH");
  if(!PATH) return;


  const char delimiter = (shell_commons::getSystemName() == "Windows") ? ';' : ':';

  vector<string> dirs;
  dirs = tokenizeString(PATH, delimiter);
  

  unordered_set<string> seen;


  for (const auto& dir : dirs){
    DIR* d = opendir(dir.c_str());
    if(!d) continue;
    struct dirent* ent;

    while((ent = readdir(d)) != nullptr){

      const char* name = ent->d_name;

      if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0){
        continue;
      }

      string fullpath = dir + "/" + name;


      if(access(fullpath.c_str(), X_OK) != 0){
        continue;
      }

      string cmd_name = name;

      if(seen.insert(cmd_name).second){
        path_executables.push_back(cmd_name);
        full_paths_of_execs.push_back(fullpath);
      }
    }

    closedir(d);
  }



  

}