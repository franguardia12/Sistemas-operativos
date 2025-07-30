#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aux.h"

int
verificar_syscall(int syscall, const char *msg)
{
	if (syscall == -1) {
		perror(msg);
		exit(EXIT_FAILURE);
	}

	return syscall;
}

void
verificar_error_apertura(int resultado_open)
{
	if (resultado_open < 0) {
		printf_debug("Error Open \n");
		exit(-1);
	}
}

void
verificar_error_dup2(int resultado_dup)
{
	if (resultado_dup < 0) {
		printf_debug("Error dup2 \n");
		exit(-1);
	}
}

void
verificar_error_pip(int pip)
{
	if (pip < 0) {
		printf_debug("Error al crear un pipe \n");
		exit(-1);
	}
}

void
verificar_error_fork(pid_t error)
{
	if (error < 0) {
		printf_debug("Error fork \n");
		exit(-1);
	}
}