#include <iostream>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

int jobIndex = 0;
int pipes = 0, elementCount = 0;
int processNum = 0, argvNum = 0;
int jobNum;
string stdinMy = "STDIN_FILENO", stdoutMy = "STDOUT_FILENO", stderrMy = "STDERR_FILENO";
bool inAppend = false;
bool outAppend = false;
bool errAppend = false;
int childExitStatus = 0;
const char * convertedArguments[99];
const char * convertedPath;
string fullArray[99];
string parsedArray[99][99];
int ** pipeNum;
int saved_stdout = dup(1);
int saved_stdin = dup(0);
int saved_stderr = dup(2);

struct jobList{
string status;
string theRest;
} tempJob;//struct storing the JobID

jobList jobArray[99];

/**
 * This function handles closing pipes
*/
void close_pipe(int pipefd [2]) {
  if (close(pipefd[0]) == -1) {
    perror("close");
  }
  if (close(pipefd[1]) == -1) {
    perror("close");
  }
}

/**
 * This function displays the jobs
 */
void displayJobs() {
  std::cout.setf(std::ios::unitbuf);
  cout << "JID" << "\t" << "STATUS" << "\t" << "COMMAND" << endl;
  for (int i = 0; i < 99; i++) {
    if (jobArray[i].status != "") {
      cout << i << "\t" << jobArray[i].status << "\t" << jobArray[i].theRest << endl;
    }
  }
}
/**
 * This function checks if the input is a builtin command or not
 */
bool checkForBuiltin(string bi) {
  std::cout.setf(std::ios::unitbuf);
  if (bi == "bg" || bi == "cd" || bi == "exit" || bi == "export" || bi == "fg" || bi == "help" || bi == "jobs") {
    return true;
  }
  else {
    return false;
  }
}
/**
 * This function resets the io redirection
 */
void resetRedir(int fdI,int fdO,int fdE){
  if (fdI == 0){
  }//if
  else{
    dup2(saved_stdin, 0);
    close(saved_stdin);
  }
  if (fdO == 0){
  }//if
  else{
    dup2(saved_stdout, 1);
    close(saved_stdout);
  }
  if (fdE == 0){
  }//if
  else{
    dup2(saved_stderr, 2);
    close(saved_stderr);
  }
}//resets all of the redirects if they had been changed

int newOutput(string file, bool outAppend){
  int fd = 0;
  if (outAppend){
    fd = open(file.c_str(), O_RDWR | O_APPEND);
    dup2(fd, 1);
  }//if output is appended
  else if(!outAppend){
    fd = open(file.c_str(), O_RDWR | O_TRUNC);
    dup2(fd, 1);
  }
  return fd;
}//newOutputAppend

int newInput(string fileIn){
  int fd = 0;

  fd = open(fileIn.c_str(), O_RDWR | O_APPEND);
  dup2(fd, 0);

  return fd;
}//setting the new input

int newErr(string file, bool errAppend){
  int fd = 0;
  if (errAppend){
    fd = open(file.c_str(), O_RDWR | O_APPEND);
    dup2(fd, 2);
  }//if output is appended
  else if(!errAppend){
    fd = open(file.c_str(), O_RDWR | O_TRUNC);
    dup2(fd, 2);
  }
  return fd;
}//setting the new error

/**
 * This function handles displaying the prompt
 */
string userPrompt() {
  std::cout.setf(std::ios::unitbuf);
  char buffer[PATH_MAX];
  getcwd(buffer, PATH_MAX); // program pwd                                               
  string cwd(buffer);
  string pwd = cwd; // get absolutepath
  string home = getenv("HOME");
  if (pwd.substr(0, home.length()) == home) {
    pwd = "~" + pwd.substr(home.length(), pwd.length() - 1);
  }
  return "1730sh:" + pwd + "$ ";
}
/**
 * This function clears the array
 */
void clearArray(){
    std::cout.setf(std::ios::unitbuf);
    for(unsigned i = 0; i < 98; i++){
        if (fullArray[i] == ""){
            break;
        }//stopping the loop iof the array runs out of elements
        fullArray[i] = "";
    }//clearing the fullArray
    for(unsigned i = 0; i < 98; i++){
        unsigned j = 0;
        if (parsedArray[i][j] == ""){
            break;
        }//stopping the loop iof the array runs out of elements
        for (j = 0; j < 98; j++){
            if (parsedArray[i][j] == ""){
                break;
            }//stopping the loop iof the array runs out of elements
            parsedArray[i][j] = "";
        }
    }//clearing the parsed array -- this doesn't work properly
    pipes = 0; elementCount = 0;
    processNum = 0; argvNum = 0;
    stdinMy = "STDIN_FILENO"; stdoutMy = "STDOUT_FILENO"; stderrMy = "STDERR_FILENO";
    inAppend = false; outAppend = false; errAppend = false;
    //clearing the bools, strings, and the counters
    
}//effectivly resets the program to prepare for new input
/**
 * This function creates jobs and executes the processes
 */
void executeCommand() {
  std::cout.setf(std::ios::unitbuf);
  bool foreground = true;
  bool background = false;
  pipeNum = new int*[(processNum)];
  for (int i = 0;i <=processNum-1; i++) {
    pipeNum[i] = new int[2];
  }
  //This is where we execute the commands that we parsed
  pid_t pid;
  for(int i = 0; i < processNum+1; i++){
    if (checkForBuiltin(parsedArray[i][0]) == true) {
      if (parsedArray[i][0] == "exit") {
        int exitS = std::stoi(parsedArray[i][1]);
        for (int i = 0; i < processNum-1; ++i) {
          delete [] pipeNum[i];
        } // for
        delete[] pipeNum;
        exit(exitS);
      }
      else if (parsedArray[i][0] == "help") {
        cout << "bg JID - Resume the stopped job JID in the background, as if it had been started with &." << endl;
        cout << "cd [PATH] - Change the current directory to PATH." << endl;
        cout << "exit [N] - Cause the shell to exit with a status of N." << endl;
        cout << "export NAME[=WORD] - NAMEis automatically included in the environment of subsequently executed jobs." << endl;
        cout << "fg JID - Resume job JID in the foreground, and make it the current job." << endl;
        cout << "help - Display helpful information about builtin commands." << endl;
        cout << "jobs - List current jobs" << endl;
        cout << "kill [-s SIGNAL] PID - The kill utility sends the specified signal to the specified process or process group PID." << endl;
      }
      else if (parsedArray[i][0] == "jobs") {
        displayJobs();
      }
      else if (parsedArray[i][0] == "cd") {
        const char * newPath = parsedArray[i][1].c_str(); // get the path user wants
        if (parsedArray[i][1] == "") {
          newPath = getenv("HOME");
        }
        if (chdir(newPath) == -1) { // change into that directory
          perror("1730sh");
        }
      }
    }
    else {
      for (unsigned int k = 0; k < parsedArray[i][0].length(); k++) {
        string tempParse = parsedArray[i][0];
        if (tempParse[parsedArray[i][0].length()-1] == '&') {
          background = true;
          foreground = false;
        }
      }
      //This is where we re-parse the command double array to a const char * so it can be entered into execvp
      for (unsigned j = 0; j < sizeof(fullArray[i]); j++) {
        if (parsedArray[i][j] == ""){
          continue;
        }
        if (j == 0) {
          convertedPath = parsedArray[i][j].c_str();
          convertedArguments[0] = parsedArray[i][j].c_str();
        }
        else {
          convertedArguments[j] = parsedArray[i][j].c_str();
        }
        
        if (j == 0 && parsedArray[i][j] == "") {
          convertedArguments[0] = (const char*)' ';
        }
      }
      if (i < processNum-1 && processNum > 1) {
        if (pipe(pipeNum[i]) == -1) {
          perror("pipe");
        }
      }
      //This is where we actually execute
      if ((pid = fork()) == -1) { // if we werent able to fork
        perror("Fork error");
        continue;
      }
      else if (pid == 0) { // in child
        // reset signals
        signal(SIGINT, SIG_DFL); 
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        int execute = execvp(convertedPath, (char * const *)convertedArguments);
        if (execute == -1) {
          cout << "1730sh: " << convertedPath << ": command not found" << endl;
        }
        if (i == 0 && processNum > 1) {
          dup2(pipeNum[i][1], STDOUT_FILENO);
          close_pipe(pipeNum[i]);
        }
        else if (i < processNum-1  && processNum > 1) {
          dup2(pipeNum[i-1][0], STDIN_FILENO);
          dup2(pipeNum[i][1], STDOUT_FILENO);
          close_pipe(pipeNum[i-1]);
          close_pipe(pipeNum[i]);
        }
        else if (i == processNum-1 && processNum > 1) {
          dup2(pipeNum[i-1][0], STDIN_FILENO);
          close_pipe(pipeNum[i-1]);
        }

        for (int i = 0; i < processNum-1; ++i) {
          delete [] pipeNum[i];
        } // for
        delete[] pipeNum;
        exit(0);
      }
      else { // in parent
        tempJob.status = "";
        tempJob.status = "Running";
        tempJob.theRest = "";
        for (unsigned j = 0; j < sizeof(fullArray[i]); j++) {
          if (parsedArray[i][j] == ""){
            continue;
          }
          if (j > 0) {
            tempJob.theRest += " ";
          }
          tempJob.theRest += parsedArray[i][j];
        }
        int exitStatus;
        if (background == true) {
          waitpid(-1, &exitStatus, WNOHANG);
        }
        else if (foreground == true) {
          waitpid(-1,&exitStatus,0);
        }
        string strES = std::to_string(WEXITSTATUS(exitStatus));
        tempJob.status = "Exited (";
        tempJob.status += strES;
        tempJob.status += ")";
        jobArray[jobIndex] = tempJob;
        displayJobs();
        jobIndex++;
      }
      if ((i > 0) && (processNum > 1)) {
        close_pipe(pipeNum[i-1]);
      }
    }
  }
  //endExecute
}

int main(int argc, const char* args[]){
    std::cout.setf(std::ios::unitbuf);
    while (0 ==0){
        stringstream ss;
        string fullInput;
        string indivInput;

        cout << userPrompt();

        getline(cin, fullInput);
        ss << fullInput;

        while(ss >> indivInput){
            fullArray[elementCount] = indivInput; 
            elementCount++;
        }//infinite until the sstream ends putting every command into an array to be parsed later

        for(unsigned i = 0; i < sizeof(fullArray); i++){
            if (fullArray[i] == ""){
                break;
            }//stopping the loop iof the array runs out of elements

            if (fullArray[i] == "|"){
                processNum++;
                pipes++;
                i++;
                argvNum = 0;

                parsedArray[processNum][argvNum] = fullArray[i];
                argvNum++;
            }//increasing the processNum if there is a pipe detected and resetting the argvCount
            else if(fullArray[i].substr(0,1) == "\""){
                int indivLength = fullArray[i].length();
                parsedArray[processNum][argvNum] = fullArray[i].substr(1);
                while(0==0){
                    indivLength = fullArray[i].length();
                    if (fullArray[i].substr(indivLength-1, indivLength) == "\"" && fullArray[i].substr(indivLength-2, indivLength) != "\\\""){
                        unsigned parsedLength = parsedArray[processNum][argvNum].length();
                        parsedArray[processNum][argvNum] = parsedArray[processNum][argvNum].substr(0, parsedLength-1);
                        break;
                    }//for when the thing terminates with an end quote. removes the endquote
                    else if(fullArray[i].substr(indivLength-1, indivLength) == "\"" && fullArray[i].substr(indivLength-2, indivLength) == "\\\""){
                        //parsedArray[processNum][argvNum] = parsedArray[processNum][argvNum] + " " + fullArray[i+1];
                    }//handling the \" s removes all the |'s
                    parsedArray[processNum][argvNum] = parsedArray[processNum][argvNum] + " " + fullArray[i+1];
                    i++;
                }//infinite loop going until the end " is detected
                argvNum++;
            }//if there are double quotes
            else if (fullArray[i] == "<" || fullArray[i] == "<<"){
                if(fullArray[i] == "<<"){
                    inAppend = true;
                }
                stdinMy = fullArray[i+1];
                i++;
                argvNum = 0;
            }//setting the STDIN stuff
            else if (fullArray[i] == ">" || fullArray[i] == ">>"){
                if(fullArray[i] == ">>"){
                    outAppend = true;
                }
                stdoutMy = fullArray[i+1];
                i++;
                argvNum = 0;
            }//setting the STDOUT stuff
            else if (fullArray[i] == "e>" || fullArray[i] == "e>>"){
                if(fullArray[i] == "e>>"){
                    errAppend = true;
                }
                stderrMy = fullArray[i+1];
                i++;
                argvNum = 0;
            }//setting the STERRstuff
            else{
                parsedArray[processNum][argvNum] = fullArray[i];
                argvNum++;
            }//if it is not a pipe or an IO redirect, sets the 2D array elements for the fullArray[]
        }//for loop sorting out each of the elements of the array into process + argv 2D arrays or setting the stdin/stdout
        
        int fdI = 0, fdO = 0, fdE = 0;

        fdI = newInput(stdinMy);
        fdO = newOutput(stdoutMy, outAppend);
        fdE = newErr(stdoutMy, errAppend); 
        
        //This will check if the code is a builtin command or not
        //this should create a pipe between each of the processes
        executeCommand();
        
        resetRedir(fdI, fdO, fdE); //reseting to std everything
        //looping through the entire parsed Array
        clearArray();
        for (int i = 0; i < (processNum-1); ++i) {
          delete [] pipeNum[i];
        } // for
        delete[] pipeNum;
    }//infinite while for taking input
  exit(EXIT_SUCCESS);
}//main