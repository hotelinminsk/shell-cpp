#include "../lib/commons/commons.hpp"
using namespace std;

string getFirstToken(const string& s, const char key,string& remainder){
  if(s.size() < 1) return " ";
  string temp, firsttoken;
  bool isfirst = true;
  for(char x : s){
    if(isfirst){
      if(x == key){
        firsttoken = temp;
        temp.clear();
        isfirst = false;
      }else{
        temp += x;
      }
    }else{
      remainder += x;
    }
  }

  if(firsttoken.size() < 1 && temp.size() > 1){
    firsttoken = temp;
  }

  
  return firsttoken;
}

vector<string> tokenizeString(const string& s, const char key){
  if(s.size() < 1) return {};
  string temp; 
  vector<string> tokens;
  for(char x : s){
    if(x == key){
      tokens.push_back(temp);
      temp.clear();
    }else{
      temp += x;
    }
  }
  if(temp.size() > 0){
    tokens.push_back(temp);
    temp.clear();
  }
  return tokens;
}



unordered_map<string, shell_commons::COMMANDTYPES> commands_and_types_map = {{"exit", shell_commons::COMMANDTYPES::BUILTIN}, {"echo", shell_commons::COMMANDTYPES::BUILTIN}, {"type", shell_commons::COMMANDTYPES::BUILTIN}};


bool isCommand_exists(string token){

  if(commands_and_types_map.count(token) > 0){
    return true;
  }

  return false;

}




string search_path_type(const string& execname, int& returncode){
  string system =  shell_commons::getSystemName();
  returncode = 0;
  string path = getenv("PATH");
  vector<string> tokenizedPaths;
  string possible = "";
  if(system == "Windows"){
    tokenizedPaths = tokenizeString(path,';');
    returncode = 0;
    for(const string& dirpath : tokenizedPaths){
      possible = dirpath + "/" + execname;
      if(access(possible.c_str(), X_OK) == 0){
        returncode = 1;
        break;
      }
    }
    return possible;
  }else if(system == "Linux"){
    tokenizedPaths = tokenizeString(path,':');
    for(const string& dirpath : tokenizedPaths){
        possible = dirpath + "/" + execname;
        if(access(possible.c_str(), X_OK) == 0){
          //it can be executable
          returncode = 1;
          break;
        }
    }
    return possible;

  }else if(system == "macOS"){

  }else {
    cout << "Invalid system type : "<< system << endl;
    
    return possible;
  }

  return possible;
}


int doJob(shell_commons::CMDS cmd,  vector<string> args, int& flag, int& returnvalue,string remainder){
    if(cmd == shell_commons::CMDS::EXIT){
      flag = true;
      returnvalue = stoi(args[0]);
      return 0;
    }else if(cmd == shell_commons::CMDS::ECHO){
      cout << remainder <<endl;
     
      return 0;
    }else if(cmd == shell_commons::CMDS::TYPE){
      string res = shell_commons::trim(remainder);
      if(commands_and_types_map.count(res)){
        if(commands_and_types_map[res] == shell_commons::COMMANDTYPES::BUILTIN){
          cout << res<< " is a shell builtin"<<endl;
        }
      }else{
        string possible = search_path_type(res, returnvalue);
        if(returnvalue == 1){
            cout << res << " is " << possible << endl;
          }else{
            cout << res << ": not found"<<endl;
          }
      }
    }
    else{
      return -1;
    }  

    return 0;
}

int main() {



  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int exitstatuscode = 0;
  int exitcalled = false;
  int functionreturncode = 0;
  int nillflag = 99;

  string command;
  while(!exitcalled){
  std::cout<<"$ ";
  getline(cin, command);
  
  vector<string> tokens = tokenizeString(command, ' ');
  string remainder = "";

  string cmd = getFirstToken(command,' ', remainder);
  // string cmd = tokens[0];

  tokens.erase(tokens.begin());

  bool isOkay = false;

  if(isCommand_exists(cmd)){

    if(cmd == "exit"){
      doJob(shell_commons::CMDS::EXIT,tokens, exitcalled, exitstatuscode,remainder);
    }else if(cmd == "echo"){
      doJob(shell_commons::CMDS::ECHO, tokens, nillflag, exitstatuscode,remainder);
    }else if (cmd == "type"){
      doJob(shell_commons::CMDS::TYPE, tokens, nillflag, functionreturncode,remainder);
    }
    else{
      cout << cmd <<":"<<" command not found" <<endl;
    }
  }else{
    cout << cmd <<":"<<" command not found" <<endl;
  }
  
  command.clear();
}

  return exitstatuscode;

  
}
