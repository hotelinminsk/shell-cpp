#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>
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



enum class CMDS {
  EXIT = 999,
  ECHO,
};

unordered_map<string, int> commands_and_arguments_map = {{"exit", 1}, {"echo", 3}};



bool isCommand_exists(string token){

  if(commands_and_arguments_map.count(token) > 0){
    return true;
  }

  return false;

}



int doJob(CMDS cmd,  vector<string> args, int& flag, int& returnvalue,string remainder){
    if(cmd == CMDS::EXIT){
      flag = true;
      returnvalue = stoi(args[0]);
      return 0;
    }else if(cmd == CMDS::ECHO){
      cout << remainder <<endl;
      // for(string key : args){
      //   cout << key;
      // }
      // cout << endl;
      return 0;
    }else{
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
      doJob(CMDS::EXIT,tokens, exitcalled, exitstatuscode,remainder);
    }else if(cmd == "echo"){
      doJob(CMDS::ECHO, tokens, nillflag, exitstatuscode,remainder);
    }else{
      cout << cmd <<":"<<" command not found" <<endl;
    }
  }else{
    cout << cmd <<":"<<" command not found" <<endl;
  }
  
  command.clear();
}

  return exitstatuscode;

  
}
