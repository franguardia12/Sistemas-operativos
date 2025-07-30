# shell

### Búsqueda en $PATH

1. ¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?
    
    1. execve(2)
        - Es una llamada al sistema directa del kernel.
        - Requiere que se especifiquen todos los argumentos y variables de entorno explícitamente.
        - Requiere la ruta completa al ejecutable.

    2. Wrappers exec(3) (execvp, execv, etc.):
        - Son funciones de alto nivel que simplifican el uso de execve.
        - Ofrece variantes más convenientes como execl, execlp, execle, execv, execvp y execvpe que manejan los argumentos y el entorno de diferentes maneras.
        - Algunas variantes como execlp y execvp, buscan automáticamente el ejecutable en las rutas definidas en la variable de entorno PATH.

2. ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
    
    1. Sí, la llamada a las funciones de la familia exec(3) puede fallar. Cuando esto ocurre, la función retorna al proceso llamante, lo cual no sucedería si la ejecución fuera exitosa. El valor de retorno es -1, además de que se establece la variable global errno para indicar la naturaleza específica del error. Algunas causas comunes del fallo son:
        - El archivo ejecutable no existe.
        - Falta de permisos para ejecutar el archivo.
        - El archivo no es un ejecutable válido.
        - Errores en la especificación de argumentos o variables de entorno.

    2. En caso de que exec(3) falle en nuestra implementación, el comportamiento sería el siguiente:

        1. Detección del error:
            
            - La función verificar_syscall detectaría que exec(3) ha retornado -1, indicando un error.
        
        2. Mensaje de error:

            - Se imprimiría un mensaje de error detallado usando perror(). Este mensaje incluiría la cadena pasada como segundo argumento a verificar_syscall.
        
        3. Terminación del proceso:
            - Inmediatamente después de imprimir el mensaje de error, la función llamaría a exit(EXIT_FAILURE), lo que terminaría el proceso hijo con un código de salida que indica fallo.


### Procesos en segundo plano

El mecanismo utilizado, sin tener en cuenta la implementación de segundo plano avanzado, es tener un wait no bloqueante que verifique si hubo un proceso en segundo plano que haya terminado y en caso de que sí se imprima su background info.

Ya manejando en segundo plano avanzado el objetivo es inicializar un stack alternativo para lograr manejar las señales dentro de el sin correr el peligro de que ocurra alguna excepcion como puede ser un stack overflow en caso de que el proceso tuviera su stack casi lleno y manejar la señal requiera mas memoria de la disponible para el proceso. A su vez, se definio una funcion *handler* que se encargara de manejar la señal *SIGCHLD* y filtrara a todos los procesos hijos que no formen parte del mismo process group id que el padre (la shell). De esta forma aseguramos que estamos tratando de un proceso que fue ejecutado en segundo plano y no un proceso normal. Se usara la funcion *sigaction* para indicar a que señal y que funcion usar para manejar dicha señal, a la vez que con *sigaltstack* se proporciona el stack alternativo. La funcion del handler no vivira dentro del stack alternativo, sino sus variables locales, el puntero de pila, y el contexto de la ejecucion. Es decir, el codigo en si estara guardado en el stack del proceso.

### Flujo estándar

El significado de ‘2>&1’ es redireccionar el file descriptor 2 (stderr) a lo mismo que esté apuntando el file descriptor 1 (stdout). Es muy útil cuando quiero enviar el output a un archivo y el error a ese mismo archivo.

Al ejecutar obtenemos (Bash):
martin@DESKTOP-1KCO2HD:~$ ls -C /home /noexiste >out.txt 2>&1
martin@DESKTOP-1KCO2HD:~$ cat out.txt
ls: cannot access '/noexiste': No such file or directory

En nuestra Shell:
$  ls -C /home /noexiste >out.txt 2>&1
        Program: [ ls -C /home /noexiste >out.txt 2>&1] exited, status: 2
$  cat out.txt
ls: cannot access '/noexiste': No such file or directory

Esto se debe a que el comando ls -C /home /noexiste >out.txt 2>&1 redirige a la salida estándar (stdout) y a la salida de error (stderr) al archivo out.txt. Cuando ls intenta acceder a /noexiste, genera un mensaje de error que se envía a out.txt. Por esto, al usar cat out.txt, se muestra ese mensaje.

En el caso de cambiar el orden (Bash):
martin@DESKTOP-1KCO2HD:~$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
martin@DESKTOP-1KCO2HD:~$ cat out.txt
ls: cannot access '/noexiste': No such file or directory

En nuestra shell:
$  ls -C /home /noexiste 2>&1 >out.txt
        Program: [ ls -C /home /noexiste 2>&1 >out.txt] exited, status: 2
 /home/martin
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory

Si cambio el orden  ls -C /home /noexiste 2>&1 >out.txt el resultado es el mismo, ya que la salida de error (stderr) es redirigida a la salida estándar (stdout) que a su vez va a apuntar a out.txt.


### Tuberías múltiples

Cuando se utiliza un pipe en la shell, el exit code que obtendremos sera el del ultimo comando en la cadena de pipes.

En caso de que alguno de los comandos del pipe falle puede ocurrir que:
    *Si los comandos no estan relacionados, y el primero falla pero el segundo no, entonces el exit code sera el de exito.
    *Si los comandos si estan relacionados (es decir, el segundo necesita algo del primero) entonces el segundo comando nos dara error ya que recibe por stdin lo que envia el primer comando por el pipe mediante su stdout.
    *Si falla el segundo comando, el exit code sera de error (independientemente del primero).
    *Si ambos comandos fallan, el exit code sera de error.
    *Si ningun comando falla, el exit code sera de exito.

Ej en Bash:
martin@DESKTOP-1KCO2HD:~$  false | true
martin@DESKTOP-1KCO2HD:~$ echo $?
0
martin@DESKTOP-1KCO2HD:~$ true | false
martin@DESKTOP-1KCO2HD:~$ echo $?
1

Como puede verse en el primer caso, al tener el último comando una salida exitosa nos devuelve 0. 
Por otro lado, en el segundo caso al tener como último comando un false, el cual siempre falla, nos devuelve un exit code de 1.

En nuestra Shell:

$ cd non_existent_directory | ls
out.txt  tps 
$ ls | cd non_existent_directory
Error execvp


### Variables de entorno temporarias

1. ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

    Porque al usar fork(), el proceso hijo hereda una copia del espacio de direcciones del proceso padre, esto incluye las variables de entorno. Entonces, si las variables de entorno se modificaran antes de llamar a fork(), el cambio afectaría al proceso padre y a cualquier otro proceso hijo que se cree después. Al realizar los cambios de variables de entorno en el proceso hijo, se garantiza que esos cambios sean locales al proceso hijo y no afecten al padre o a otros procesos.

2. ¿El comportamiento resultante es el mismo que en el primer caso si se usa una de las funciones de exec(3) con el tercer argumento?

    No, el comportamiento no es el mismo. Las funciones de la familia exec(3) que permiten pasar un tercer argumento para las variables de entorno (por ejemplo, execle(), execvpe()) permiten especificar directamente el conjunto de variables de entorno que estarán disponibles para el programa ejecutado. Es decir, reemplaza completamente el entorno del proceso con el conjunto de variables que se pasa. Si se usa setenv(3), las variables nuevas se agregan o modifican en el entorno actual, pero no reemplazan completamente el entorno.

3. Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.
    
    Una posible implementación sería:
    
    1. Llamar a fork(2) para crear un proceso hijo.

    2. En el proceso hijo, crear un arreglo incluyendo todas las variables de entorno necesarias, tanto las nuevas como las preexistentes.
    
    3. Pasar ese arreglo al tercer argumento de una función de exec(3) (por ejemplo, execvpe()).

### Pseudo-variables

1. Describir el propósito de la variable mágica `?`.
    
    - Esta variable, dentro de una shell, almacena el estado de salida (status code) del último comando ejecutado. Este valor es importante para saber si el comando anterior se ejecutó con éxito (estado 0) o falló (estado distinto de 0).

2. Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

    1. `$#`: Representa el número de argumentos pasados al script o función.

        Ejemplo:
        
        ```bash
        #!/bin/bash
        echo "Número de argumentos: $#"
        ```
        Si se ejecuta el script como ./script.sh arg1 arg2, imprimirá "Número de argumentos: 2".

    2. `$0`: Contiene el nombre del script o el comando actual. Es útil para mostrar el nombre del script dentro de sí mismo, lo que permite que sea más reutilizable o para mensajes de error.

        Ejemplo:
        
        ```bash
        #!/bin/bash
        echo "Este script se llama $0"
        ```

        Si se ejecuta ./script.sh, imprimirá "Este script se llama ./script.sh".

    3. $$: Contiene el ID del proceso (PID) del shell o script actual. Es útil cuando necesitas crear identificadores únicos, como archivos temporales, usando el PID.

        Ejemplo:

        ```bash
        #!/bin/bash
        echo "El PID de este script es $$"
        ```

        Si se ejecuta ./script.sh, imprimirá "El PID de este script es *PID del proceso actual que está ejecutando el script*".

### Comandos built-in

En el caso de cd no sería posible porque, para lograr el efecto deseado, es necesario que sea un comando que se ejecute en el proceso de la Shell y no en uno nuevo, ya que no es posible que otro proceso pueda modificar el directorio de la Shell y, en lugar de eso, modificaría su directorio y eso no es el efecto esperado.

En el caso de pwd sí se podría implementar sin necesidad de ser built-in ya que el proceso hijo que se cree luego de hacer fork y exec sigue teniendo el mismo directorio que el proceso padre (que es el proceso de la Shell), por lo que podría ejecutar pwd igual y funcionaría.

El motivo de implementar pwd como un comando built-in es por una cuestión de performance, ya que el proceso de la Shell ya conoce cual es su directorio actual y no es necesario crear un proceso nuevo, lo cual es costoso en recursos. Además por razones de consistencia ya que la shell ya tiene la información del directorio actual a través de la variable interna PWD, lo que puede evitar discrepancias cuando se usan symlinks. Por este motivo es que se termina implementando como un comando built-in.

### Señales

#### ¿Por qué es necesario el uso de señales?

El uso de  señales es lo que permite a los procesos comunicarse entre si sobre eventos que ocurren en los mismos. Son necesarias para poder manejar eficiente y controladamente la ejecucion de un proceso de manera que no sea necesario estar consultando todo el tiempo si se recibio una señal o no. Estos se manejan de forma asincrona y permiten que un proceso pueda enviar una señal a otro proceso para que este realice una accion determinada y que una vez manejada la señal, el proceso pueda seguir con su ejecucion normal.

### Historial

---
