#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		if (block_contains(eargv[i], '=') != -1) {
			char *nombre_var = strtok(eargv[i], "=");
			char *valor = strtok(NULL, "=");

			if (nombre_var && valor) {
				setenv(nombre_var, valor, 1);
			}
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags & O_CREAT) {  //& --> AND bit a bit
		return open(file, flags, S_IRUSR | S_IWUSR);  //| --> OR bit a bit
	} else {
		return open(file, flags);
	}

	return -1;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;

		if (e->eargc > 0) {
			set_environ_vars(e->eargv, e->eargc);
		}

		verificar_syscall(execvp(e->argv[0], e->argv),
		                  "Error al ejecutar el programa.");

		break;
	}

	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		if (strlen(r->out_file) > 0) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_CLOEXEC | O_CREAT |
			                                   O_TRUNC | O_WRONLY);
			verificar_error_apertura(fd_out);
			int dup_out = dup2(fd_out, 1);
			verificar_error_dup2(dup_out);
		}
		if (strlen(r->in_file) > 0) {
			int fd_in =
			        open_redir_fd(r->in_file, O_CLOEXEC | O_RDONLY);
			verificar_error_apertura(fd_in);
			int dup_in = dup2(fd_in, 0);
			verificar_error_dup2(dup_in);
		}
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				int dup_pasaje =
				        dup2(1, 2);  // redirijo stderr a stdout
				verificar_error_dup2(dup_pasaje);
			} else {
				int fd_err =
				        open_redir_fd(r->err_file,
				                      O_CLOEXEC | O_WRONLY |
				                              O_CREAT | O_TRUNC);
				verificar_error_apertura(fd_err);
				int dup_errr = dup2(fd_err, 2);
				verificar_error_dup2(dup_errr);
			}
		}
		r->type = EXEC;
		exec_cmd((struct cmd *) r);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;  // Casteo
		int fds[2];
		int pip = pipe(fds);
		verificar_error_pip(pip);
		pid_t hijo_izq = fork();
		verificar_error_fork(hijo_izq);

		if (hijo_izq == 0) {
			close(fds[0]);
			int dup_izq = dup2(fds[1], 1);
			verificar_error_dup2(dup_izq);
			// Aca no usamos el flag del open ==> Tenemos que cerrar nosotros
			close(fds[1]);
			exec_cmd((struct cmd *) p->leftcmd);
			exit(-1);
		}
		pid_t hijo_der = fork();
		verificar_error_fork(hijo_der);

		if (hijo_der == 0) {
			close(fds[1]);
			int dup_der = dup2(fds[0], 0);
			verificar_error_dup2(dup_der);
			close(fds[0]);
			exec_cmd((struct cmd *) p->rightcmd);
			exit(-1);
		}

		close(fds[1]);
		close(fds[0]);
		int status;
		waitpid(hijo_izq, &status, 0);
		waitpid(hijo_der, &status, 0);


		free_command(parsed_pipe);
		_exit(EXIT_SUCCESS);  // Discord --> Necesario para que pase test 13
		break;
	}
	}
}
