#ifndef AUX_H
#define AUX_H
#define PRIMER_ARG 0
#define SEGUNDO_ARG 1
#define INTERROGACION '?'
#define DOLAR '$'
#define CARACTER_CERO '0'

int verificar_syscall(int syscall, const char *msg);

void verificar_error_apertura(int resultado_open);

void verificar_error_dup2(int resultado_dup);

void verificar_error_pip(int pip);

void verificar_error_fork(pid_t error);

#endif  // AUX_H
