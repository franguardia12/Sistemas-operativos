#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void
criba(int fd_lectura)
{
	int n;
	if (read(fd_lectura, &n, sizeof(int)) <= 0) {
		close(fd_lectura);
		return;
	}
	printf("primo %i\n", n);
	fflush(stdout);

	int fds[2];
	if (pipe(fds) == -1) {
		perror("pipe error");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork error");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		// hijo
		close(fds[1]);
		close(fd_lectura);
		criba(fds[0]);
		exit(EXIT_SUCCESS);
	} else {
		// padre
		close(fds[0]);
		int num;
		while (read(fd_lectura, &num, sizeof(int)) > 0) {
			if (num % n != 0) {
				if (write(fds[1], &num, sizeof(int)) == -1) {
					perror("write error");
					exit(EXIT_FAILURE);
				}
			}
		}
		close(fds[1]);
		close(fd_lectura);
		wait(NULL);
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Cantidad de argumentos invalida\n");
		exit(EXIT_FAILURE);
	}

	int n = atoi(argv[1]);

	int fds[2];
	if (pipe(fds) == -1) {
		perror("pipe error");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork error");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		// hijo
		close(fds[1]);
		criba(fds[0]);
		exit(EXIT_SUCCESS);
	} else {
		// padre
		close(fds[0]);
		for (int i = 2; i <= n; i++) {
			if (write(fds[1], &i, sizeof(int)) == -1) {
				perror("write error");
				exit(EXIT_FAILURE);
			}
		}
		close(fds[1]);
		wait(NULL);
	}

	return 0;
}