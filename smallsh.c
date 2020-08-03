#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

// global variables for status function
int status = 0;
int lastForegroundExit = 1;
int lastForegroundTerminated = 0;

// global variables for shell input
int foregroundOnly = 0;
int validInput = 0;

// custom handler for SIGTSTP (CTRL Z)
// toggles shell from foreground only mode, to foreground and background mode
void handle_SIGTSTP(int signo){

	// if in foreground and background mode, switch to foreground only mode
	if (foregroundOnly == 0){
		foregroundOnly = 1;
		write(1, "\nEntering foreground-only mode (& is now ignored)\n", 50);
		fflush(stdout);

	// if in foreground only mode, switch to foreground and background mode
	} else if (foregroundOnly == 1){
		foregroundOnly = 0;
		write(1, "\nExiting foreground-only mode\n", 30);
		fflush(stdout);
	}

}


// kills all processes shell has started and exits shell
int shellExit(){

	// kill all processes shell has started	
	kill(0, SIGKILL);

	// exit shell
	printf("exiting shell my pid is %d\n", getpid());
	fflush(stdout);
	exit(0);

	return 0;

}

// change directory to path environment HOME
int shellCd(){

	chdir(getenv("HOME"));

	return 0;
}

// change directory to passed path
int shellCdPath(char* path){

	chdir(path);

	return 0;
}

// display most recent process exit value or terminating signal
int shellStatus(){
		
	// if most recent process was exited, display exit value
	if (lastForegroundExit == 1){
		printf("exit value %d\n", status);
		fflush(stdout);
	}

	// if most recent process was terminated by signal, display signal number
	if (lastForegroundTerminated == 1){
		printf("terminated by signal %d\n", status);
		fflush(stdout);
	}

	return 0;
}

// handles output redirection for foreground and background processes, displays errors if needed
int errorOutputRedirection(char* outputStream, int isForeground){

	// allocate memory
	int targetOutputStream;
	int targetOutputResult;

	// if background process and no output given
	if (!isForeground && strcmp(outputStream, "") == 0){
		
		targetOutputStream = open("/dev/null", O_WRONLY);
			
		// if output destination cannot be open for write only, display error
		if (targetOutputStream == -1){		
			perror("open output:"); 
			fflush(stdout);
			return 1;
		}
		
		targetOutputResult = dup2(targetOutputStream, 1);
		
		// if file descriptor wasnt copied correctly, display error
		if (targetOutputResult == -1){
			perror(""); 
			fflush(stdout);
			return 1;
		}

		return 0;
		
	}

	
	// if a non empty output stream was passed
	if (strcmp(outputStream, "") != 0){

		// open output for write only, create if needed, if exists remove existing content
		targetOutputStream = open(outputStream, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	
		//if output destination cannot be open for write only, display error
		if (targetOutputStream == -1){		
			perror("open output:"); 
			fflush(stdout);
			return 1;
		}
		
		targetOutputResult = dup2(targetOutputStream, 1);
		
		// if file descriptor wasnt copied correctly, display error
		if (targetOutputResult == -1){
			perror(""); 
			fflush(stdout);
			return 1;
		}

	}

	return 0;

}


// handles input redirection for foreground and background processes, displays errors if needed
int errorInputRedirection(char* inputStream, int isForeground){
	
	// allocate memory
	int targetInputStream;
	int targetInputResult;

	// if background process and no input  given
	if (!isForeground && strcmp(inputStream, "") == 0){
	
		targetInputStream = open("/dev/null", O_RDONLY);
			
		// if cannot open source for read only, display error
		if (targetInputStream == -1){		
			perror("cannot open %s for input\n");
			fflush(stdout);
			return 1;
		}
		
		targetInputResult = dup2(targetInputStream, 1);
		
		// if a file descriptor wasnt copied correctly, display error
		if (targetInputResult == -1){
			perror(""); 
			fflush(stdout);
			return 1;
		}

		return 0;
		
	}

	// if a non empty input stream was passed
	if (strcmp(inputStream, "") != 0){

		targetInputStream = open(inputStream, O_RDONLY);

		// if an error occurs when opening source, display error
		if (targetInputStream == -1){
			printf("cannot open %s for input\n", inputStream);
			fflush(stdout);
			return 1;
		}

		targetInputResult = dup2(targetInputStream, 0);
		
		// if a file descriptor wasnt copied correctly, display error
		if (targetInputResult == -1){
			perror(""); 
			fflush(stdout);
			return 1;
		}		
	}

	return 0;

}

// executes non built in shell command by running default linux command
// accepts parameters, input redirection, ouput redirection, foreground processes and background processes
int execGeneralCommand(char* command, char** parameters, char* inputStream, char* outputStream, int isForeground){

	// fork process to create new child
	pid_t spawnPid = fork();
	int childStatus;

	// for foreground child
	if (isForeground && (spawnPid == 0 )){
	
		// set default sigint (CTRL C) response for child
		struct sigaction SIGINT_action;
		SIGINT_action.sa_handler = SIG_DFL;
		sigfillset(&SIGINT_action.sa_mask);
		SIGINT_action.sa_flags = 0;
		sigaction(SIGINT, &SIGINT_action, NULL);

	}

	// for all children
	if (spawnPid == 0){

		// ignore SIGTSTP signal (CTRL Z) response for child
		struct sigaction SIGTSTP_action;
		SIGTSTP_action.sa_handler = SIG_IGN;
		sigfillset(&SIGTSTP_action.sa_mask);
		SIGTSTP_action.sa_flags = 0;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	}


	switch(spawnPid){
			
		// handle child creation error
		case -1:
			
			perror("fork()\n");
			fflush(stdout);
			exit(1);
			break;
			
		// handle successul child creation
		case 0:
	
			// handle input redirection and if needed error display
			if (errorInputRedirection(inputStream, isForeground) == 1){
				exit(1);
				break;
			}

			// handle output redirection and if needed error display
			if (errorOutputRedirection(outputStream, isForeground) == 1){
				exit(1);
				break;
			}
			
			// set up and run default linux command
			parameters[0] = command;
			execvp(parameters[0], parameters);

			// error when child executed
			printf("%s: no such file or directory\n", command);
			fflush(stdout);
			exit(1);
			break;

		// parent process 
		default:

			// if background process
			if (!isForeground){
				printf("background pid is %d\n", spawnPid);
				fflush(stdout);

				// allow new shell input while command is running
				spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
			}

			// if foreground process
			if (isForeground){

				// do not allow new shell input while command is running
				spawnPid = waitpid(spawnPid, &childStatus, 0);
				
				// if child process was exited normally
				if (WIFEXITED(childStatus)){
					status = WEXITSTATUS(childStatus);
					lastForegroundExit = 1;
					lastForegroundTerminated = 0;
				}

				// if child process was terminated by signal
				if (WIFSIGNALED(childStatus)){
					status = WTERMSIG(childStatus);
					lastForegroundExit = 0;
					lastForegroundTerminated = 1;
					printf("terminated by signal %d\n" , WTERMSIG(childStatus));
					fflush(stdout);
				}
			}

			break;	
	
	}

	return 0;
}

// gets user input and sets gloal variable for validInput
char* getUserInput(){

	// assume input will be valid
	validInput = 1;	

	// allocate memory
	char* userInput;
 	size_t size = 2048;
	userInput = (char *) malloc (size);

	write(1, ": ", 2);
	fflush(stdout);

	int numCharsEntered = getline(&userInput, &size, stdin);	

	// if invalid input, set global variable
	if (numCharsEntered == -1){
		clearerr(stdin);
		validInput = 0;
	}		
	
	return userInput;	
}

// process user input for blank input, comments, built in commands (exit, status, cd),
// input redirection, output redirection, foreground process, background process,
// and variable expansion ($$ -> shell pid)
void processUserInput(char* userInput){

	// allocate space for shell pid for variable expansion
	char shellID[255];
	sprintf(shellID, "%d", getpid());

	// allocate space and copy passed user input
	char* copyUserInput;
	size_t size = 2048;
	copyUserInput = (char *) malloc (size);
	strcpy(copyUserInput, userInput);

	// allocate space for variables related to execute general command function
	char userCommand[2048];
	char* userParams[513];
	char inputStream[2048];
	char outputStream[2048];
	int isForeground = 1;
	int paramsIndex = 1;
	strcpy(userCommand, "");
	strcpy(inputStream, "");
	strcpy(outputStream, "");

	// set up parsing for copied userInput
	char* token = strtok(copyUserInput, " \n");

	// reprompt user if input is blank
	if (!token){
		return;
	}

	// reprompt user if input is a comment
	if (strncmp("#", token, 1) == 0){
		return;
	}

	// process first token which is assumed a command
	strcpy(userCommand, token);
	token = strtok(NULL, " \n");

	// check if command is  built in command exit
	if (strcmp("exit", userCommand) == 0){
		free(userInput);
		free(copyUserInput);
		shellExit();
		return;
	}

	// check if command is built in command status
	if (strcmp("status", userCommand) == 0){
		shellStatus();
		free(copyUserInput);
		return; 
	}

	// check if command is built in commmand  cd and no following parameter
	if (strcmp("cd", userCommand) == 0 && (!token)){

		shellCd();
		
		free(copyUserInput);
		
		return;
	}	

	// while a parameter, input/output redirection or foreground/background process input exists
	while(token){

		// process input redirection	
		if (strcmp("<", token) == 0){
			token = strtok(NULL, " \n");
			strcat(inputStream, token);

		// process output redirection
		} else if (strcmp(">", token) == 0){
			token = strtok(NULL, " \n");
			strcat(outputStream, token);

		// process run command in background
		} else if (strcmp("&", token) == 0){
			isForeground = 0;

		// process variable expansion ($$ -> shell pid)
		} else if (strcmp("$$", token) == 0){
			userParams[paramsIndex] = shellID;
			paramsIndex += 1;

		// process all other tokens
		} else {

			// allocate space for second token and formatted token
			char *token2;
			token2 = token;
			char formattedToken[255];
			strcpy(formattedToken, "");

			// set pidPtr to first instance of $$ and all trailing text
			char *pidPtr;	
			pidPtr = strstr(token2, "$$");

			// while token contains $$, perform variable expanson
			while (pidPtr != NULL){

				// add command before $$
				strncat(formattedToken, token2, strlen(token2)-strlen(pidPtr));

				// expand $$ -> shell pid
				strcat(formattedToken, shellID);

				// set token2 to next command and all trailing $$ and text
				token2 = pidPtr + 2;
				
				// set pidPtr to next instance of $$ and all trailing text
				pidPtr = strstr(token2, "$$");

			}

			// if no variable expansion occured	
			if (strcmp(formattedToken, "") == 0){

				userParams[paramsIndex] = token;
	
			// if variable expansion occured
			} else {
	
				// add any test after final $$
				strcat(formattedToken, token2);

				userParams[paramsIndex] = formattedToken;
			}

			paramsIndex += 1;
		}

		token = strtok(NULL, " \n");
	}

	// if cd is first command and a parameter exists
	if (strcmp("cd", userCommand) == 0){

		shellCdPath(userParams[1]);		
		free(copyUserInput);
	
		return;

	}
	

	// set up usserParams for execvp function in execute general command function
	userParams[paramsIndex] = NULL;

	// overwrite user & to ignore background command when shell is in foreground only mode
	if (foregroundOnly == 1){
		isForeground = 1;
	}	

	//execute general command
	execGeneralCommand(userCommand, userParams, inputStream, outputStream, isForeground);

	free(copyUserInput);
	
	return;
}

int main (void) {

	// set up shell to ignore SIGINT (CTRL C)
	struct sigaction SIGINT_action;
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// set up shell with custom  SIGTSTP handler (CTRL Z)
	struct sigaction SIGTSTP_action;
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// set pointer for user input 
	char* userInput;

	while (1){

		pid_t spawnPid;
		int childStatus;

		// if a background process has completed
		while ((spawnPid = waitpid(-1, &childStatus, WNOHANG)) > 0){

			// if if foreground and background mode
			if (!foregroundOnly){

				// display pid and exit status
				if (WIFEXITED(childStatus)){
					printf("background pid %d is done: exit value %d\n", spawnPid, WEXITSTATUS(childStatus));
					fflush(stdout);

				}

				// display pid and signal termination number
				if (WIFSIGNALED(childStatus)){
					printf("background pid %d is done: terminated by signal %d\n ", spawnPid, WTERMSIG(childStatus));
					fflush(stdout);
				}
			}

		}
	
		userInput = getUserInput();

		if (validInput){
			processUserInput(userInput);
		}

		free(userInput);

	}

	return 0;
}
