#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t pid = fork();

	if (pid == 0) {
		cprintf("Prioridad del proceso hijo: %d\n", sys_get_priority());
	} else {
		cprintf("Prioridad del proceso padre: %d\n", sys_get_priority());
	}
}