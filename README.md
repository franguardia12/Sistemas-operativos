# Sistemas-operativos
Trabajos realizados en la materia sistemas operativos de la facultad de ingeniería (UBA). Cátedra Mendez 2C2024.

# Trabajos individuales

- fork: trabajo individual introductorio de la materia en el que se pone en práctica el uso de la syscall fork y el uso de pipes resolviendo dos ejercicios: primes y xargs. En primes se calculan todos los números primos menores a un número natural n utilizando el algoritmo de criba de Eratóstenes, y en xargs se implementa la utilidad del mismo nombre que permite ejecutar un binario una o más veces pasándole como argumentos los leídos desde stdin

# Trabajos grupales

Integrantes: Franco Guardia (@franguardia12), Joseph Mamani (@J0SEPHMT), Martín Castro (@Martiin17), Lucas García (@Sh4rkzDev)

- shell: implementación en C de la funcionalidad mínima de un intérprete de comandos shell similar a lo que realizan bash, zsh o fish
- sched: implementación en C del mecanismo de cambio de contexto para procesos y el scheduler (planificador) sobre un sistema operativo preexistente. Se utiliza un kernel que es una modificación de JOS, un exokernel educativo con licencia libre del grupo Sistemas Operativos Distribuidos del MIT
- filesystem: implementación en C de un sistema de archivos (filesystem) propio para Linux enteramente en memoria. Este sistema de archivos utilizará el mecanismo de FUSE (Filesystem in USEr space) provisto por el kernel que permite definir en modo usuario la implementación de un filesystem
