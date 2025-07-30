#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define MIN_TICKETS 10
#define MAX_SCHED_EJECUCIONES 8
#define MAX_PROCESSES 100000
#define TICKET_LOWER_LIMIT 0

int amount_sched_calls = 0;
int sched_total_executions = 0;
unsigned int pid[MAX_PROCESSES];
unsigned int pid_index = 0;
unsigned int env_runs[MAX_PROCESSES];

void sched_halt(void);

static unsigned random_seed = 1;

unsigned
lcg_parkmiller(unsigned *state)
{
	const unsigned N = 0x7fffffff;
	const unsigned G = 48271u;
	unsigned div =
	        *state / (N / G); /* max : 2,147,483,646 / 44,488 = 48,271 */
	unsigned rem =
	        *state % (N / G); /* max : 2,147,483,646 % 44,488 = 44,487 */

	unsigned a = rem * G;       /* max : 44,487 * 48,271 = 2,147,431,977 */
	unsigned b = div * (N % G); /* max : 48,271 * 3,399 = 164,073,129 */

	return *state = (a > b) ? (a - b) : (a + (N - b));
}

unsigned
next_random()
{
	return lcg_parkmiller(&random_seed);
}

void
show_statistics()
{
	cprintf("Estadisticas del sistema:\n");

	// Mostrar el número total de tickets asignados a cada proceso
	int i = 0;
	while (envs[i].env_id) {
		cprintf("Proceso %d:\n", envs[i].env_id);
		cprintf("- Tickets asignados: %d\n", envs[i].tickets);
		cprintf("- Veces ejecutado: %d\n", envs[i].env_runs);
		cprintf("- CPU en la que se ejecuto: %d\n", envs[i].env_cpunum);
		i++;
	}

	// Mostrar el número total de llamadas al scheduler
	cprintf("Numero total de llamadas al scheduler: %d\n", amount_sched_calls);

	// Mostrar el número total de ejecuciones de procesos
	cprintf("Numero total de ejecuciones de procesos: %d\n",
	        sched_total_executions);
}

// Aumenta los tickets de todos los procesos RUNNABLE al mínimo
void
boost(void)
{
	for (int i = 0; i < NENV; i++) {
		if (envs[i].env_status == ENV_RUNNABLE) {
			envs[i].tickets = MIN_TICKETS;
		}
	}
}

// Calcula el total de tickets de todos los procesos RUNNABLE
int
calculate_total_tickets(void)
{
	int total_tickets = 0;
	for (int i = 0; i < NENV; i++) {
		if (envs[i].env_status == ENV_RUNNABLE) {
			total_tickets += envs[i].tickets;
		}
	}
	return total_tickets;
}

// Maneja el caso cuando no hay tickets disponibles
void
handle_no_tickets(struct Env *current_env)
{
	if (current_env && current_env->env_status == ENV_RUNNING) {
		sched_total_executions++;
		pid[pid_index++] = current_env->env_id;
		if (current_env->tickets > MIN_TICKETS)
			current_env->tickets--;
		env_run(current_env);
	} else {
		sched_halt();
	}
}

// Selecciona los procesos candidatos para ejecutar
int
select_candidate_processes(struct Env *envs_to_run[], int total_tickets)
{
	int winning_ticket = next_random() % total_tickets;
	int ticket_sum = 0;
	int envs_to_run_total = 0;

	for (int i = 0; i < NENV; i++) {
		ticket_sum += envs[i].tickets;
		if (envs[i].env_status == ENV_RUNNABLE &&
		    ticket_sum > winning_ticket) {
			int j = 0;
			while (j < envs_to_run_total) {
				if (envs_to_run[j]->tickets != envs[i].tickets) {
					break;
				}
				j++;
			}
			if (j == envs_to_run_total) {
				envs_to_run[envs_to_run_total] = &envs[i];
				envs_to_run_total++;
			}
		}
	}
	return envs_to_run_total;
}

// Ejecuta el proceso ganador final
void
run_winning_process(struct Env *envs_to_run[], int envs_to_run_total)
{
	if (envs_to_run_total > 0) {
		int final_winner = next_random() % envs_to_run_total;
		struct Env *final_env = envs_to_run[final_winner];

		if (final_env->tickets > MIN_TICKETS)
			final_env->tickets--;

		sched_total_executions++;
		pid[pid_index++] = final_env->env_id;
		env_run(final_env);
	}
}

void
lotery(void)
{
	struct Env *current_env = curenv;
	struct Env *envs_to_run[NENV];

	if (sched_total_executions % MAX_SCHED_EJECUCIONES == 0) {
		boost();
	}

	int total_tickets = calculate_total_tickets();

	if (total_tickets == TICKET_LOWER_LIMIT) {
		handle_no_tickets(current_env);
		return;
	}

	int envs_to_run_total =
	        select_candidate_processes(envs_to_run, total_tickets);
	run_winning_process(envs_to_run, envs_to_run_total);
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here - Round robin

	int i = 0;
	if (curenv) {
		i = ENVX(curenv->env_id);
	}
	int j = i;
	do {
		i = (i + 1) % NENV;
		if (envs[i].env_status == ENV_RUNNABLE) {
			env_run(&envs[i]);
		}
	} while (i != j);

	if (curenv && curenv->env_status == ENV_RUNNING) {
		sched_total_executions++;
		env_run(curenv);
	}

	sched_halt();

#endif

#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	// Your code here - Priorities

	lotery();

	if (curenv && curenv->env_status == ENV_RUNNING &&
	    curenv->env_cpunum == cpunum()) {
		sched_total_executions++;
		env_run(curenv);
	}

#endif

	// Without scheduler, keep runing the last environment while it exists
	/* if (curenv) {
	        env_run(curenv);
	} */

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		show_statistics();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here


	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
