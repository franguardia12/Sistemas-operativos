#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };
void *global_ss_sp;

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

static void
signal_handler()
{
	pid_t pid;
	int status;
	pid = waitpid(0, &status, WNOHANG);
	if (pid > 0) {
		printf("Child process [%d] finished with status %d\n", pid, status);
	}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	// Configuring the signal handler with its own stack
	stack_t ss;
	ss.ss_sp = malloc(SIGSTKSZ);
	if (ss.ss_sp == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	ss.ss_size = SIGSTKSZ;
	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL) == -1) {
		perror("sigaltstack");
		exit(EXIT_FAILURE);
	}
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_ONSTACK | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	// Store the stack pointer globally for later cleanup
	global_ss_sp = ss.ss_sp;
}

int
main(void)
{
	init_shell();

	run_shell();

	// Free the allocated memory before exiting
	free(global_ss_sp);

	return 0;
}
