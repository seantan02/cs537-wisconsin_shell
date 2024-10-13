#include "stdio.h" 
#include "sys/wait.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "dirent.h"
#include "fcntl.h"

#define MAXLINE 1024
#define MAXARGS 256

// definition of build in commands index
#define EXIT 0
#define CD 1
#define EXPORT 2
#define LOCAL 3
#define VARS 4
#define HISTORY 5
#define LS 6

// Linked List
struct localVar {
  char *name;
  char *value;
  
  struct localVar *next;
};

// global variables
int returnResult = 0;
char cmdInput[MAXLINE];
int ori_stdin;
int ori_stdout;
int ori_stderr;
// global localVar
struct localVar *localVariables = NULL;
int historyCurr = 0;
int historySize = 5;
char **history = NULL; 


// localVariable functions
extern int setlocal(char *name, char *value, int overwrite);
extern char* getlocal(char *name);
extern void setFreeLocalVariables();

int builtItCommandCalled = 0;  // 0 means no built-in command called in this attmept; 1 means yes.
int byPassBuiltInCommandStop = 0; // 1 means continue executiong even after built-in command called.

// Function declarations
extern void built_in_exit(char **arguments);
extern void built_in_cd(char **arguments);
extern void built_in_export(char **arguments);
extern void built_in_local(char **arguments);
extern void built_in_vars(char **arguments);
extern void built_in_history(char **arguments);
extern void built_in_ls(char **arguments);
extern void parseArguments(char **arguments, char *input, const char *delimeter, int maxarg, int *argcPointer);
extern int adjustHistorySize(int n);
extern int cmpDirents(const void *p1, const void *p2);

// Built-in commands table
void (*builtInCommands[])(char**) = {
built_in_exit,
built_in_cd,
built_in_export,
built_in_local,
built_in_vars,
built_in_history,
built_in_ls,
};

void builtInCommand(int builtInCommandIndex, char **arguments){
	builtInCommands[builtInCommandIndex](arguments);
	builtItCommandCalled = 1;
}

// built-in commands definitions
void built_in_exit(char **arguments){
  // free all arguments and exit
  if(arguments != NULL){
	free(arguments);
	arguments = NULL;
  }

  // free all history
  if(history != NULL){
	for(int i=0; i<historySize; i++){
	  if(*(history+i) != NULL){
		free(*(history+i));
		*(history+i) = NULL;
	  }
	}
	free(history);
	history = NULL;
  }

  setFreeLocalVariables(); 
 
  if(returnResult) exit(-1);
  exit(0);
}

void built_in_cd(char **arguments){
  if(*(arguments+2) != NULL) exit(-1);  // If more than 1 argument or 3rd arugment is not NULL
  const char *path = *(arguments+1); // first argument is the path
  if(chdir(path) != 0) exit(-1);
}

void built_in_export(char **arguments){
  // if there's no arguments
  if(*(arguments+1) == NULL || strlen((*(arguments+1))) == 0){
	returnResult = -1;
	built_in_exit(arguments);
  }
  // if there's nothing after = then we clear the variable
  int variableAssignmentLen = strlen(*(arguments+1));
  if((*(arguments+1))[variableAssignmentLen-1] == '='){
    char *variable = calloc(variableAssignmentLen+1, sizeof(char));
    strcpy(variable, *(arguments+1));
    variable[variableAssignmentLen-1] = '\0'; // remove =
	int setEnvStat = setenv(variable, "", 1);
    free(variable);  // free memory
	variable = NULL;

	if(setEnvStat != 0){
	  returnResult = -1;
	  built_in_exit(arguments);
	};  // If error occurs when removing env var
    return;  // we are done
  }

  char **input = calloc(2, sizeof(char*));
  char *token;
  token = strtok(*(arguments+1), "=");
  int i = 0;
  while(i < 2){
    if(token==NULL) exit(-1);  // No = found
    *(input+i) = token;
    token = strtok(NULL, "=");
    i++;
  }

  // convert right hand side to variable if it is one
  char *trueVal = NULL;
  if((*(input+1))[0] == '$'){
	int valueLen = strlen(*(input+1));
	char inputWithoutDollar[valueLen+1];
	strcpy(inputWithoutDollar, (*(input+1))+1);
	trueVal = getenv(inputWithoutDollar);
	if(trueVal == NULL || strlen(trueVal) == 0) trueVal = getlocal(*(input+1)); 
  }else{
	trueVal = *(input+1);
  }

  int setenvStat;
  if(trueVal != NULL && strlen(trueVal) > 0) setenvStat = setenv(*(input+0), trueVal, 1);
  else setenvStat = setenv(*(input+0), "", 1);

  free(input);  // free allocated mem
  input = NULL;

  if(setenvStat != 0){
	returnResult = -1;
	built_in_exit(arguments);
  }
}

void built_in_local(char **arguments){
  // if no arguments
  if(*(arguments+1) == NULL || strlen((*(arguments+1))) == 0){
	returnResult = -1;
	built_in_exit(arguments);
  }
  // if there's nothing after = then we clear the variable
  int variableAssignmentLen = strlen(*(arguments+1));
  if((*(arguments+1))[variableAssignmentLen-1] == '='){
	char *variable = calloc(variableAssignmentLen+1, sizeof(char));
	strcpy(variable, *(arguments+1));
	variable[variableAssignmentLen-1] = '\0'; // remove =
	int setLocalStat = setlocal(variable, "", 1);
	free(variable);
	variable = NULL;

	if(setLocalStat != 0){
	  returnResult = -1;
	  built_in_exit(arguments);
	};  // If error occurs when removing env var
	return;  // we are done
  }

  char **input = calloc(2, sizeof(char*));
  char *token;
  token = strtok(*(arguments+1), "=");
  int i = 0;
  while(i < 2){
	if(token==NULL) exit(-1);  // No = found
	*(input+i) = token;
	token = strtok(NULL, "=");
	i++;
  }

  char *trueVal = NULL;
  if((*(input+1))[0] == '$'){
    int valueLen = strlen(*(input+1));
    char inputWithoutDollar[valueLen+1];
    strcpy(inputWithoutDollar, (*(input+1))+1);
    trueVal = getenv(inputWithoutDollar);
	if(trueVal == NULL || strlen(trueVal) == 0) trueVal = getlocal(*(input+1));
  }else{
	trueVal = *(input+1);
  }
 
  int setlocalStat;
  if(trueVal != NULL && strlen(trueVal) > 0) setlocalStat = setlocal(*(input+0), trueVal, 1);
  else setlocalStat = setlocal(*(input+0), "", 1);

  free(input);
  input = NULL;

  if(setlocalStat != 0){
	returnResult = -1;
	built_in_exit(arguments);
  };
}

void built_in_vars(char **arguments){
  if(*(arguments+1) != NULL) exit(-1);
  if(localVariables == NULL) return;

  struct localVar *temp = localVariables;
  while(temp != NULL){
	printf("%s=%s\n", temp->name, temp->value);
	temp = temp->next;
  }
}

void built_in_history(char **arguments){
  if(*(arguments+1) == NULL){
    if(history == NULL) return; // nothing to print
	int number = 1;
	for(int i=historyCurr-1; i>-1; i--){
	  // we skip it if it's null or empty
	  if(history[i] == NULL) continue;
	  if(strlen(history[i]) == 0) continue;
	  printf("%i) %s\n", number, history[i]);
	  number++;
	}
	return;
  }

  if(*(arguments+2) == NULL){
	int n = atoi(*(arguments+1));
	if(n < 1) return; // does nothing if not valid
	if (n > historySize) return; // over the capacity
	
	
	// reverse n to reflect which one it is
	if(history[historySize-1] != NULL){
	  n = (n-historySize)*-1; 
	}else{
	  // loop through to find the reverse nth element
	  for(int i=historyCurr-1; i>-1; i--){
	    if(--n == 0){
			n=i;
			break;
		} 
	  }
	}
	
	if(history[n] == NULL){  // if command invalid
	  return;
	}

	strcpy(cmdInput, history[n]);
	parseArguments(arguments, cmdInput, " ", MAXARGS, NULL);
	byPassBuiltInCommandStop=1;  // it will allow the arguments to flow through
	return;
  }

  if(strcmp(*(arguments+1), "set") == 0){
	if(*(arguments+2) == NULL) return;
	int n = atoi(*(arguments+2));
	if(n < 1) return;
	int adjustHistStat = adjustHistorySize(n);
	// there's error
	if(adjustHistStat != 0){
	  returnResult = -1;
	  built_in_exit(arguments);
	}
	return;
  }

}

void built_in_ls(char **arguments){
  
  struct dirent **namelist;
  int n;

  char *dirPath;
  if(*(arguments+1) == NULL) dirPath = strdup(".");
  else dirPath = strdup(*(arguments+1));

  n = scandir(dirPath, &namelist, NULL, alphasort);
  if (n == -1) {
	free(dirPath);
	dirPath = NULL;
	returnResult = -1;
	built_in_exit(arguments);
  }
  
  qsort(namelist, n, sizeof(struct dirent *), cmpDirents);

  int i = -1;
  while (++i < n) {
	if(namelist[i]->d_name == NULL) continue;
	if((namelist[i]->d_name)[0] == '.') continue;
	printf("%s\n", namelist[i]->d_name);
  }
  
  // free all memory before finishing
  i = -1;
  while (++i < n) {
	free(namelist[i]);
	namelist[i] = NULL;
  }

  free(namelist);
  namelist = NULL;
  free(dirPath);
  dirPath = NULL;
}

// local variable functions
char* getlocal(char *name){
  if(localVariables == NULL) return strdup(""); // Nothing in local variable
  // Go through the linked list and look for the variable
  char *nameWithoutDollar = calloc(strlen(name), sizeof(char));  // strlen only bc we're copying 1less
  strcpy(nameWithoutDollar, name+1); 
  struct localVar *temp = localVariables;
  while(temp != NULL){
	if(strcmp(temp->name, nameWithoutDollar) == 0){
	  free(nameWithoutDollar);
	  nameWithoutDollar = NULL;
	  return temp->value;
	}
	temp = temp->next;
  }

  // Nothing found
  if(nameWithoutDollar != NULL){
	free(nameWithoutDollar);
    nameWithoutDollar = NULL;
  }
  
  return NULL;
}

int setlocal(char *name, char *value, int overwrite){
  /*
  1. Go through our linked list and see if we can find the variable
  2. If found, check overwrite == 1;
  3. If overwrite == 1, overwrite the value, done.
  4. Else overwrite != 1 we are done.
  5. Else (not found), create one and create a new next node
  */

  // First time using setlocal
  struct localVar *temp = localVariables;
  struct localVar *prev = NULL;

  // go through the link list
  while(temp != NULL){
    if(strcmp(temp->name, name) == 0){
	  if(overwrite != 1) return 0;  // no overwrite so we are done
      temp->value = realloc(temp->value, sizeof(char)*strlen(value)+1);
	  strcpy(temp->value, value);
      return 0;
    }
    // go to next node every loop
    prev = temp;
	temp = temp->next;
  }
   
  // If we are at the last loop and we reach here, we will create a new node
  struct localVar *newNode = (struct localVar*)calloc(1, sizeof(struct localVar));
  if(newNode == NULL) return -1;
  newNode->name = malloc(sizeof(char)*strlen(name)+1);
  if(newNode->name == NULL) return -1;
  strcpy(newNode->name, name);
  newNode->value = malloc(sizeof(char)*strlen(value)+1);
  if(newNode->value == NULL) return -1;
  strcpy(newNode->value, value);
  newNode->next = NULL;
  
  if(localVariables == NULL){
	localVariables = newNode;
  }else{
	prev->next = newNode;
  }
  return 0;
}


// Helpers function
void parseArguments(char **arguments, char *input, const char *delimeter, int maxarg, int *argcPointer){
  char *inputToken;
  int argumentCount = 0;
  inputToken = strtok(input, delimeter);
  while(inputToken != NULL && argumentCount < maxarg){  // If there's more token to read
    int tokenLen = strlen(inputToken);  // It's O(N) so we should not recall it
    if(inputToken[tokenLen-1] == '\n'){
	  inputToken[tokenLen-1] = '\0';  // Remove newline char
	}
	// Check if there's a $ sign at the start
	if(inputToken[0] == '$'){
	  char tokenWithoutDollar[tokenLen+1];
	  strcpy(tokenWithoutDollar, inputToken+1); // no dollar sign
	  char *envVar = getenv(tokenWithoutDollar);
	  if(envVar == NULL || strlen(envVar) == 0) inputToken = getlocal(inputToken);
	  else inputToken = envVar;
	}

	*(arguments+argumentCount) = inputToken;
    inputToken = strtok(NULL, delimeter);
	argumentCount++;
  }
  if(argcPointer != NULL) *argcPointer = argumentCount;  // set the count for caller
  *(arguments+argumentCount) = NULL; // Add a NULL at the end
}


char* getValidBinPaths(char *command){
  /*
  This function will read PATH env var and split by :
  Then starting from index 0 since adding PATH is export PATH=NEW:$PATH
  Then it uses access("PATH/command", X_OK) to check if the Path is valid
  Return the full executable path with command if found; Null otherwise.
  */
  char *paths = strdup(getenv("PATH"));
  if(paths == NULL) return NULL;

  char *path = strtok(paths, ":");
  char *result = NULL;

  while(path != NULL){
	int pathLen = strlen(path);
	int commandLen = strlen(command);
	char *fullPath = calloc(pathLen+commandLen+2, sizeof(char));  //We need to add a '/'
	if(fullPath == NULL) exit(-1);
	strcpy(fullPath, path);
	fullPath[pathLen] = '/';
	strcpy(fullPath+pathLen+1, command);
	
	if(access(fullPath, X_OK) == 0){
	  result = fullPath;
	  break;
	}

	if(fullPath != NULL){
	  free(fullPath);
	  fullPath = NULL;
	}

	path = strtok(NULL, ":"); //next
  }

  if(paths != NULL){
	free(paths);
	paths = NULL;
  }

  return result;
}


void setFreeLocalVariables(){
  struct localVar *temp = localVariables;
  struct localVar *next = NULL;

  while(temp != NULL){
	next = temp->next;
	// free name, value then go next
    if(temp->name != NULL) free(temp->name);
	if(temp->value != NULL) free(temp->value);
	temp->name = NULL;
	temp->value = NULL;
	free(temp);
    temp = next;
  }
  localVariables = NULL;

}


void addToHistory(char **arguments){
  if(history == NULL){  // first time setting a history then we allocate memory for it
	history = calloc(historySize, sizeof(char*));
	if(history == NULL) exit(-1);
  }

  if(historyCurr == historySize){ // Full so we move the list backward one and reduce historyCurr by 1
	free(history[0]); // remove first one (oldest)
	history[0] = NULL;
    for(int i=0; i<historySize-1; i++){
	  history[i] = history[i+1]; //shift to left
	}
	historyCurr--;
  }
  // Copy arguments over
  char *historyInput = calloc(MAXLINE, sizeof(char));
  int indexToAddAt = 0;
  int i = 0;
  while(*(arguments+i) != NULL && i < MAXARGS){
	int currArgLen = strlen(*(arguments+i));
	if(i > 0){
	  if(indexToAddAt+1 < MAXLINE){
	  historyInput[indexToAddAt] = ' '; // add a space
	  indexToAddAt++;  // +1 after space
	  }else{
	    break;
	  }
	}

	if(indexToAddAt+currArgLen < MAXLINE){
	  strcpy(historyInput+indexToAddAt, *(arguments+i));
	  indexToAddAt = indexToAddAt+currArgLen;
	}else{
	  break;
	}
	i++;
  }

  history[historyCurr] = historyInput; //record in history
  historyCurr++;
}


int adjustHistorySize(int n){
  if(history == NULL){
	history = calloc(n, sizeof(char*));  // allocate and we're done
	if(history == NULL){
	  return -1;
	}
	historySize = n;
	return 0;
  }

  if(historySize == n) return 0; // same size then we do nothing

  if(n < historySize){
	// we only need to kick out elements if it exceeds new size
	int avaibility = historySize-historyCurr;  // curr is index and always < size
	// only perform if avaibility is less than to kick
	if(avaibility < historySize-n){
	  for(int i=0; i<historySize-n-avaibility; i++){
	    if(history[i] != NULL){
		  free(history[i]);
		  history[i] = NULL;
	    }
	  }
	  // now we need to shift it to left
	  for(int i=0; i<n; i++){
	    history[i] = history[i+historySize-n-avaibility];
	    history[i+historySize-n-avaibility] = NULL;  // clear it after
	  }
	
	  historyCurr = n; // set the pointer to the last element
	}
  }
  // if we get to this line that means n > historySize
  history = realloc(history, n*sizeof(char*));
  if(history == NULL){
	return -1;
  }
  // if n > old size we will need to make sure new allocated memory is NULL
  if(n > historySize){
    for(int i=historySize; i<n; i++){
	  history[i] = NULL;
    }
  }

  historySize = n;
  return 0;
}


// Compare function
int cmpDirents(const void *p1, const void *p2)
{
	const struct dirent *dir1 = *(const struct dirent **)p1;
	const struct dirent *dir2 = *(const struct dirent **)p2;

	int cmp =  strcmp(dir1->d_name, dir2->d_name);
	return cmp;
}

int main(int argc, char ** argv){

  if(argc > 2) exit(0);

  FILE *inputStream = stdin;
  if(argc == 2){
	char *filename = strtok(argv[1], "\n");
	inputStream = fopen(filename, "r");
    if(inputStream == NULL) exit(-1);
  }

  const char delimeter[2] = " ";

  char **arguments = NULL;
  int argumentsCount = 0;

  // initialize stdin, stdout, stderr for restoration
  ori_stdin = dup(STDIN_FILENO);
  ori_stdout = dup(STDOUT_FILENO);
  ori_stderr = dup(STDERR_FILENO);

  // set execution PATH
  if(setenv("PATH", "/bin", 1) != 0) exit(-1);  // overwrite

  while(1){
	// If to exit then exit
	if(inputStream == stdin){
	  printf("wsh> ");
	  fflush(stdout);
	}

	if(fgets(cmdInput, MAXLINE, inputStream) == NULL) built_in_exit(arguments);
	if(feof(inputStream)) built_in_exit(arguments);  // EOF reached

    // Now we want to split the input by space and store them all as arguments
    // Note that MAXARGS + 1 because we need a NULL at the end
    arguments = calloc(MAXARGS+1, sizeof(char*)); // allocate memory to store all arguments user input
	if(arguments == NULL) exit(-1);  // Failed to allocate memory
	parseArguments(arguments, cmdInput, delimeter, MAXARGS, &argumentsCount);  // Parse user input into arguments

	// If there's no arguments, continue
    if(*(arguments) == NULL || strlen(*(arguments)) == 0 || (*arguments)[0] == '#'){
	  if(arguments != NULL) free(arguments);
	  arguments = NULL; 
	  continue;
	};

	// redirection
	int file;
	// redirection of >
	char *pointerToStartOfRedirection = strchr(*(arguments+argumentsCount-1), '>');
	if(pointerToStartOfRedirection != NULL && pointerToStartOfRedirection[0] == '>'){
	  char *outputFileName;
	  *pointerToStartOfRedirection = '\0'; // to extract the number
	  int fileDescriptorN = atoi(*(arguments+argumentsCount-1));
	  *pointerToStartOfRedirection = '>'; // add > back
	  // if no n is given then we use STDOUT
	  if(fileDescriptorN == 0 && (*(arguments+argumentsCount-1))[0] != '0'){
		fileDescriptorN = STDOUT_FILENO;
		if((*(arguments+argumentsCount-1))[0] != '>' && (*(arguments+argumentsCount-1))[0] != '&'){
		  returnResult = -1;
		  built_in_exit(arguments);
		}
	  }else{
		fileDescriptorN = fileDescriptorN;
	  }

	  // Check if it's > or >>
	  if(pointerToStartOfRedirection[1] == '>'){ // 
		outputFileName = strdup(pointerToStartOfRedirection+2);
		file = open(outputFileName, O_WRONLY|O_CREAT|O_APPEND, 0666);
	  }else{
		outputFileName = strdup(pointerToStartOfRedirection+1);
		file = open(outputFileName, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		
	  }

	  if(outputFileName != NULL) free(outputFileName); // strdup returns new string
	  outputFileName = NULL;

	  if (dup2(file, fileDescriptorN) == -1){  // manipulating stdout failed then we exit(-1)
		returnResult = -1;
		built_in_exit(arguments);
	  }
	  
	  // if & exist
	  if((*(arguments+argumentsCount-1))[0] == '&'){
	    if (dup2(file, STDERR_FILENO) == -1){
		  returnResult = -1;
		  built_in_exit(arguments);
		}
	  }
	  close(file);

	  *(arguments+argumentsCount-1) = NULL;
	
	}else{
	  pointerToStartOfRedirection = strchr(*(arguments+argumentsCount-1), '<');
	  if(pointerToStartOfRedirection != NULL){
		*pointerToStartOfRedirection = '\0'; // to extract the number
        int fileDescriptorN = atoi(*(arguments+argumentsCount-1));
        *pointerToStartOfRedirection = '<'; // add > back
        // if no n is given then we use STDOUT
        if(fileDescriptorN == 0 && (*(arguments+argumentsCount-1))[0] != '0'){
          fileDescriptorN = STDIN_FILENO;
		  if((*(arguments+argumentsCount-1))[0] != '<'){
			returnResult = -1;
			built_in_exit(arguments);
		  }
		}else{
          fileDescriptorN = fileDescriptorN;
        }

		char *inputFileName = strdup((*(arguments+argumentsCount-1))+1);
		// Learned from stackoverflow
		file = open(inputFileName, O_RDONLY);
		if(inputFileName != NULL) free(inputFileName);
		inputFileName = NULL;

		if (dup2(file, fileDescriptorN) == -1) {
		  returnResult = -1;
		  built_in_exit(arguments);
		}
		close(file);
	    *(arguments+argumentsCount-1) = NULL;
	  }
	}

    // Built-in commands
    if(strcmp(*arguments, "exit") == 0) builtInCommand(EXIT, arguments);
    if(strcmp(*arguments, "cd") == 0) builtInCommand(CD, arguments);
    if(strcmp(*arguments, "export") == 0) builtInCommand(EXPORT, arguments);
	if(strcmp(*arguments, "local") == 0) builtInCommand(LOCAL, arguments);
	if(strcmp(*arguments, "vars") == 0) builtInCommand(VARS, arguments);
	if(strcmp(*arguments, "history") == 0) builtInCommand(HISTORY, arguments);
	if(strcmp(*arguments, "ls") == 0) builtInCommand(LS, arguments);

	// If built-in commands called then we continue to next loop without forking and exec
	if(builtItCommandCalled ){
	  builtItCommandCalled = 0;
	  if(byPassBuiltInCommandStop == 0){
		if(arguments != NULL){
		  free(arguments);
		  arguments = NULL;
		}
		continue;
	  }
	  byPassBuiltInCommandStop = 0;
	}else{
	  addToHistory(arguments);
	}

    // Everything below here should not be executed if user type "exit"
    // fork and execv here
    int forkPid = fork();
    char *commandPath = NULL;
	if(forkPid < 0) exit(-1);
    else if(forkPid == 0){
      // This is the child
	  if(access(*(arguments), X_OK) == 0){
		returnResult = 0; // no error
		execv(*(arguments), arguments);  // If the given execution is valid
	    returnResult = -1;
		built_in_exit(arguments);
	  }

	  commandPath = getValidBinPaths(*(arguments));
	  if(commandPath == NULL){
	    returnResult = -1;
		built_in_exit(arguments);
	  }

	  returnResult = 0; // no error
	  execv(commandPath, arguments);
	  free(commandPath);
	  returnResult = -1;
	  built_in_exit(arguments);
	}else{
	  if(waitpid(forkPid, &returnResult, 0) == -1){
		perror("waitpid failed");
		returnResult = -1;
		built_in_exit(arguments);
	  }

	  if(arguments != NULL){
		free(arguments);
		arguments = NULL;
	  }
	  // clear the input
	  strcpy(cmdInput, "");
	  // restore stdin, stdout, stderr
	  int restoreStdin = dup2(ori_stdin, STDIN_FILENO);
	  int restoreStdout = dup2(ori_stdout, STDOUT_FILENO);
	  int restoreStderr = dup2(ori_stderr, STDERR_FILENO);
	  if(restoreStdin == -1 || restoreStdout == -1 || restoreStderr == -1){
		returnResult = -1;
		built_in_exit(arguments);
	  }
	}
  }
  setFreeLocalVariables();
  return returnResult;
}
