#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Prioridad inicial: %d.\n", sys_get_priority());

	int32_t valor = 0;
	int cant_reducciones = 0;
	while (valor == 0) {
		valor = sys_lower_priority();
		cant_reducciones++;
	}

	cprintf("Prioridad final tras reducir la prioridad %d veces: %d\n",
	        cant_reducciones,
	        sys_get_priority());
}
