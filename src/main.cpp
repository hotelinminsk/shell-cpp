#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

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

unordered_map<string, int> commands_and_arguments_map = {{"exit", 1}};



bool isCommand_exists(string token){

  if(commands_and_arguments_map.count(token) > 0){
    return true;
  }

  return false;

}


int doJob(string cmd, vector<string> args){
    if(cmd == "exit"){
      return 999;
    }

    return 0;
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int exitstatuscode = 0;
  bool exitcalled = false;

  string command;
  while(!exitcalled){
  std::cout<<"$ ";
  getline(cin, command);
  
  vector<string> tokens = tokenizeString(command, ' ');

  string cmd = tokens[0];

  tokens.erase(tokens.begin());

  bool isOkay = false;

  if(isCommand_exists(cmd)){
    if(tokens.size() == commands_and_arguments_map[cmd]){
        int ret = doJob(cmd, tokens);
        if(ret == 999){
          exitstatuscode = stoi(tokens[0]);
          exitcalled = 1;
        }
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
