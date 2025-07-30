#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	char *word = "exit";
	if (strstr(cmd, word) != NULL) {
		return 1;
	}


	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *word = "cd";
	if (strstr(cmd, word) != NULL) {
		char *token = strtok(cmd, " ");
		token = strtok(NULL, " ");
		if (token == NULL) {
			char *home = getenv("HOME");
			return chdir(home) + 1;
		} else {
			return chdir(token) + 1;
		}
	}


	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	char *ptr = getcwd(prompt, PRMTLEN);
	char *word = "pwd";
	if (strstr(cmd, word) != NULL) {
		printf("%s\n", ptr);
		return 1;
	}


	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
