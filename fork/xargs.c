#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef NARGS
#define NARGS 4
#endif

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Cantidad de argumentos invalida\n");
		exit(EXIT_FAILURE);
	}

	char *binario = argv[1];
	char *linea = NULL;
	size_t len = 0;
	ssize_t nread;

	while (1) {
		char *args[NARGS + 2];
		args[0] = binario;

		for (int j = 1; j <= NARGS + 1; j++) {
			args[j] = NULL;
		}

		int i;
		for (i = 1; i <= NARGS; i++) {
			nread = getline(&linea, &len, stdin);
			if (nread == -1) {
				break;
			}
			if (linea[nread - 1] == '\n') {
				linea[nread - 1] = '\0';
			}
			args[i] = strdup(linea);
		}


		if (i == 1) {
			break;
		}

		pid_t pid = fork();

		if (pid == -1) {
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			// hijo
			execvp(binario, args);
			perror("execvp error");
			exit(EXIT_FAILURE);
		} else {
			// padre
			wait(NULL);
		}


		for (int j = 1; j < i; j++) {
			free(args[j]);
		}

		if (nread == -1) {
			break;
		}
	}

	free(linea);
	return 0;
}
