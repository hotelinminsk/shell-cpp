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




void init_cwd(){
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    current_working_dir = cwd;
  } else {
    perror("getcwd() error");
    exit(1);
  }
}




void handle_piped_entries(const vector<string>& left, const vector<string>& right, int& exitcalled, int& returnvalue){
  bool left_is_builtin = isBuiltin(left[0]);
  bool right_is_builtin = isBuiltin(right[0]);
  
  //left
  const string leftcmd = left[0];
  vector<string> args_only_left(left.begin() + 1, left.end()); 
  shell_commons::RedirInfo redirinfoleft;
  vector<string> clean_args_left;
  checkRedir(redirinfoleft, args_only_left,clean_args_left);

  //right 
  const string rightcmd = right[0];
  vector<string> args_only_right(right.begin() + 1, right.end()); 
  shell_commons::RedirInfo redirinforight;
  vector<string> clean_args_right;
  checkRedir(redirinforight, args_only_right,clean_args_right);


  int pd[2];
  if(pipe(pd)){
    perror("Pipe error in piped entries");
    _exit(1);
  }


  //handle redirects 
  // int left_original_out = dup(STDOUT_FILENO);
  // int left_original_errout = dup(STDERR_FILENO);
  if(left_is_builtin){
    //handleBuiltin(leftcmd, clean_args_left, redirinfoleft, exitcalled, returnvalue);
    if(redirinfoleft.stderr_redir || redirinfoleft.stdout_redir){
      // solda redir var,
      if(redirinfoleft.stdout_redir){
        int flags = O_WRONLY | O_CREAT;
        flags |= redirinfoleft.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND  : O_TRUNC;
        int fd = open(redirinfoleft.filename.c_str(), flags, 0644);
        if(fd<1) cout << "Cant open file: "<<redirinfoleft.filename<<endl;
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }else if(redirinfoleft.stderr_redir){
        int flags = O_WRONLY | O_CREAT;
        flags |= redirinfoleft.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND  : O_TRUNC;
        int fd = open(redirinfoleft.filename.c_str(), flags, 0644);
        if(fd<1) cout << "Cant open file: "<<redirinfoleft.filename<<endl;
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
      
      //output pipe a gitmeli redirect olsun olmasın, ve handle builtin çağıramıyorum çünkü 
      // o sadece redirecte bakıyor tekrar cmdleri felan yazmam lazım
     

    }else{
      // solda hiç redir yok

    }

    dup2(pd[0], STDOUT_FILENO); // output is pd[0]

    if(leftcmd == "echo"){
    for (size_t i = 0; i < clean_args_left.size(); ++i) {
        if (i > 0) cout << ' ';
        cout << clean_args_left[i];
    }
    cout << '\n';
  }else if(leftcmd == "type"){
    if(clean_args_left[0].empty()) return;
    const string searchedcommand = clean_args_left[0]; 
    if(isBuiltin(searchedcommand)){
      cout <<searchedcommand<<" is a shell builtin"<<endl; 
    }else{
      const string path = findExecutable(searchedcommand);
      if(path.empty()) {
        cout << searchedcommand <<": not found"<<endl;
      }else{
        cout << searchedcommand<< " is "<<path<<endl;
      }

    }
  }else if(leftcmd == "cd"){
    command_CD(clean_args_left[0]);
  }else if(leftcmd == "pwd"){
    command_PWD();
  }else if(leftcmd == "exit"){
    exitcalled = 1;
    if(clean_args_left.empty()){
      returnvalue = 0;
    }else{
      try{
        returnvalue = stoi(clean_args_left[0].c_str());
      }catch(exception ex){
        cerr << ex.what() << endl;
        _exit(1);
      }
    }
  }
    
  }else if(!left_is_builtin){

    if(redirinfoleft.stderr_redir || redirinfoleft.stdout_redir){
      // solda redir var,
      if(redirinfoleft.stdout_redir){
        int flags = O_WRONLY | O_CREAT;
        flags |= redirinfoleft.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND  : O_TRUNC;
        int fd = open(redirinfoleft.filename.c_str(), flags, 0644);
        if(fd<1) cout << "Cant open file: "<<redirinfoleft.filename<<endl;
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }else if(redirinfoleft.stderr_redir){
        int flags = O_WRONLY | O_CREAT;
        flags |= redirinfoleft.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND  : O_TRUNC;
        int fd = open(redirinfoleft.filename.c_str(), flags, 0644);
        if(fd<1) cout << "Cant open file: "<<redirinfoleft.filename<<endl;
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
      
      //output pipe a gitmeli redirect olsun olmasın, ve handle builtin çağıramıyorum çünkü 
      // o sadece redirecte bakıyor tekrar cmdleri felan yazmam lazım
     

    }else{
      // solda hiç redir yok

    }
    string path = findExecutable(leftcmd);
    if(path.empty()){
      cerr<< leftcmd<< ": command not found"<<endl;
      return;
    }

    vector<char*> argv;
    argv.insert(argv.begin(), const_cast<char*>(leftcmd.c_str()));
    for(auto& c : clean_args_left) argv.push_back(const_cast<char*>(c.c_str()));
    argv.push_back(nullptr);
    
    pid_t pid = fork();
    if(pid == 0 ){
      dup2(pd[0], STDOUT_FILENO);

      execv(path.c_str(), argv.data());
      perror("Execv failed.");
      _exit(1);

    }else{
      int status;
      waitpid(pid, &status, 0);
    }

  }

  if(right_is_builtin){
    //not implementend yed
    //handleBuiltin(rightcmd, clean_args_right, redirinforight, exitcalled, returnvalue);

  }else if(!right_is_builtin){
    //will implement 
    //handleExec(rightcmd, clean_args_right, redirinforight);

  }




  if(left_is_builtin && right_is_builtin){
    // both are builtins
    // we need to create a pipe between two builtins
    // not implemented yet
  }else if(left_is_builtin && !right_is_builtin){
    // left is builtin, right is not
    // we need to create a pipe from builtin to executable
    // not implemented yet
  }else if(!left_is_builtin && right_is_builtin){
    // left is not builtin, right is builtin
    // we need to create a pipe from executable to builtin
    // not implemented yet
  }else{
    // both are not builtins
    //execWithPipe(left.size(), left, right.size(), right);
  }
}

void checkRedir(shell_commons::RedirInfo& redirinfo,
                const std::vector<std::string>& args,
                std::vector<std::string>& clean_args)
{
    clean_args.clear();
    redirinfo.stdout_redir = false;
    redirinfo.stderr_redir = false;
    redirinfo.type = shell_commons::REDIRECTTYPE::NONE;
    redirinfo.filename.clear();

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& token = args[i];

        // Hiç '>' içermiyorsa, olduğu gibi argümanlara ekle
        std::size_t op_pos = token.find('>');
        if (op_pos == std::string::npos) {
            clean_args.push_back(token);
            continue;
        }

        // Buraya geldiysek token bir redirection içeriyor
        // redirinfo.stdout_redir = true;

        // FD (1 veya 2) var mı diye bak
        int fd = 1;
        std::size_t start_op = op_pos;

        // Örn: "2>file" veya "1>>out"
        // op_pos == 1 ve token[0] 1 veya 2 ise FD olarak yorumla
        if (op_pos == 1 && (token[0] == '1' || token[0] == '2')) {
            fd = token[0] - '0';
        }

        if(fd == 1) redirinfo.stdout_redir = true;
        if(fd == 2) redirinfo.stderr_redir = true;

        // append mi overwrite mı?
        bool is_append = false;
        if (token.compare(op_pos, 2, ">>") == 0) {
            is_append = true;
            redirinfo.type = shell_commons::REDIRECTTYPE::APPEND;
        } else {
            redirinfo.type = shell_commons::REDIRECTTYPE::OVERWRITE;
        }

        // Dosya adını token içinden almaya çalış
        std::size_t fname_pos = op_pos + (is_append ? 2 : 1);
        std::string filename;

        if (fname_pos < token.size()) {
            // Örn: "2>>log.txt" veya ">>out"
            filename = token.substr(fname_pos);
        } else {
            // Örn: "2>>" "log.txt" veya "1>" "out"
            if (i + 1 < args.size()) {
                filename = args[i + 1];
                // Bir sonrakini de tükettiğimiz için onu loop'ta atlayalım
                ++i;
            }
        }

        if (!filename.empty()) {
            redirinfo.filename = filename;
        }

        // Bu token (ve varsa bir sonraki filename) clean_args'e eklenmez.
        // Ama teorik olarak operator'den önce bir "prefix" olsaydı, onu eklemek isteyebilirdik.
        // Örn: "foo2>out" gibi; bu projede buna ihtiyaç yok varsayıyoruz.
    }
}


void handleExec(const string& cmd,const vector<string>& clean_args,const shell_commons::RedirInfo& redirinfo){
    string path = findExecutable(cmd);
    if(path.empty()){
      cerr << cmd << ": command not found"<<endl;
      return;
    }
    vector<char*> argv;
    argv.insert(argv.begin(), const_cast<char*>(cmd.c_str()));
    for(auto& a: clean_args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if(pid == 0){
      if(redirinfo.stderr_redir || redirinfo.stdout_redir){
        int flags =  O_WRONLY | O_CREAT;
        flags |= redirinfo.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND : O_TRUNC;
        int fd = open(redirinfo.filename.c_str(), flags, 0644);
        if(fd < 1){
          perror("open error in handleEXec");
          _exit(1);
        }

        int redirtype = redirinfo.stderr_redir ? STDERR_FILENO : STDOUT_FILENO;

        dup2(fd, redirtype);
        close(fd);
      }

      execv(path.c_str(), argv.data());
      perror("Execv failed.");
      _exit(1);
    }else{
      int status;

      waitpid(pid,&status, 0);
    }

    return;
}


void handle_single_command(const vector<string>& tokens, int& exitcalled, int& returnvalue){
    if (tokens.empty()) return;

    const string cmd = tokens[0];

    // cmd'den sonrasını argüman olarak düşün
    vector<string> args_only(tokens.begin() + 1, tokens.end());

    vector<string> clean_args;
    shell_commons::RedirInfo redirinfo;
    checkRedir(redirinfo, args_only, clean_args); // DİKKAT: tokens değil, args_only!

    if (isBuiltin(cmd)) {
        handleBuiltin(cmd, clean_args, redirinfo, exitcalled, returnvalue);
    } else {
        handleExec(cmd, clean_args, redirinfo);
    }
}

void handleBuiltin(const string& cmd, const vector<string>& clean_args, const shell_commons::RedirInfo& redirinfo, int& exitcalled, int& returnvalue){
  int oldout = dup(STDOUT_FILENO);
  int olderrout = dup(STDERR_FILENO);
  if(redirinfo.stdout_redir || redirinfo.stderr_redir){
    if(redirinfo.stdout_redir){
      int flags = O_WRONLY | O_CREAT;
      flags |= redirinfo.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND : O_TRUNC;
      int fd = open(redirinfo.filename.c_str(), flags, 0644);
      if(fd<1) cout << "Cant open file : "<< redirinfo.filename << endl;
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }else if(redirinfo.stderr_redir){
      int flags = O_WRONLY | O_CREAT;
      flags |= redirinfo.type == shell_commons::REDIRECTTYPE::APPEND ? O_APPEND : O_TRUNC;
      int fd = open(redirinfo.filename.c_str(), flags, 0644);
      if(fd<1) cout << "Cant open file: "<<redirinfo.filename<<endl;
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
  }

  if(cmd == "echo"){
    for (size_t i = 0; i < clean_args.size(); ++i) {
        if (i > 0) cout << ' ';
        cout << clean_args[i];
    }
    cout << '\n';
  }else if(cmd == "type"){
    if(clean_args[0].empty()) return;
    const string searchedcommand = clean_args[0]; 
    if(isBuiltin(searchedcommand)){
      cout <<searchedcommand<<" is a shell builtin"<<endl; 
    }else{
      const string path = findExecutable(searchedcommand);
      if(path.empty()) {
        cout << searchedcommand <<": not found"<<endl;
      }else{
        cout << searchedcommand<< " is "<<path<<endl;
      }

    }
  }else if(cmd == "cd"){
    command_CD(clean_args[0]);
  }else if(cmd == "pwd"){
    command_PWD();
  }else if(cmd == "exit"){
    exitcalled = 1;
    if(clean_args.empty()){
      returnvalue = 0;
    }else{
      try{
        returnvalue = stoi(clean_args[0].c_str());
      }catch(exception ex){
        cerr << ex.what() << endl;
        _exit(1);
      }
    }
  }

  fflush(stdout);
  fflush(stderr);

  dup2(oldout,STDOUT_FILENO);
  dup2(olderrout, STDERR_FILENO);

  close(oldout);
  close(olderrout);
  return;

}

void lineSemantics(const string& line, int& exitcalled, int& returnvalue){
  vector<string> tokens = tokenizeString(line, ' ');
  int pipepos = -1;
  for(int i= 0; i< tokens.size(); i++){
    if(tokens[i] == "|"){
      pipepos= i;
      break;
    }
  }

  if(pipepos == 0){
    perror("Pipe cant be on first.");
    return;
  }

  if(pipepos != -1){
    vector<string>leftofpipe(tokens.begin(), tokens.begin() + pipepos);
    vector<string>rightofpipe(tokens.begin() + pipepos + 1, tokens.end());
    handle_piped_entries(leftofpipe, rightofpipe, exitcalled, returnvalue);
    return;
  }

  //this checks for redirs,
  handle_single_command(tokens, exitcalled, returnvalue);
  return;

}


int main() {
    build_path_exec_cache();
    rl_bind_key('\t', rl_complete);
    rl_attempted_completion_function = my_completion;
    rl_completion_append_character = ' ';
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