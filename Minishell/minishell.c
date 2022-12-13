#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include <errno.h>

// Funcion encargada de comprobar si lo introducido por el usuario es un comando interno de la minishell
int comprobarInternos(tline *line)
{

    // Se comprueba si el usuario a introducido el mandato exit
    if (!strcmp(line->commands[line->ncommands - 1].argv[0], "exit"))
    {
        printf("----Saliendo...----\n");
        exit(0);
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato cd
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "cd"))
    {
        cd(line);
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato fg
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "fg"))
    {

        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato jobs
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "jobs"))
    {

        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato umask
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "umask"))
    {

        return 1;
    }
    return 0;
}



//Función que correcoge el codigo relacionado con el comando cd
int cd(tline *line)
{
    char *dir;
	char buffer[512];
	
	if(line->commands[0].argc > 2)
	{
	  fprintf(stderr,"No se pueden introudcir más de dos argumentos\n");
	  return 1;
	}
	
	else if (line->commands[0].argc == 1)
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
		  fprintf(stderr,"No existe la variable $HOME\n");	
          return 1;
		}
	}
	else 
	{
		dir = line->commands[0].argv[1];
	}
	
	// Comprobar si es un directorio
	if (chdir(dir) != 0) {
			fprintf(stderr,"Error al cambiar de directorio: %s\n", strerror(errno));  
            return 1;
	}

	return 0;
}

int jobs()
{
}
int fg()
{
}
int umask()
{
}

int ejecutarComandoExterno(tline * line){
    pid_t  pid;
	int status;		
		
	pid = fork();
	if (pid < 0) { /* Error */
		fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
		exit(1);
	}
	else if (pid == 0) { /* Proceso Hijo */
		execvp(line->commands[0].filename, line->commands[0].argv);
		//Si llega aquí es que se ha producido un error en el execvp
		printf("Error al ejecutar el comando: %s\n", strerror(errno));
		return 1;
		
	}
	else { /* Proceso Padre. 
    		- WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal (caida, matado con un kill, etc). 
		Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit()
    		- WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la 
		macro anterior indique que la salida ha sido por una llamada a exit(). */
		wait (&status);
		if (WIFEXITED(status) != 0)
			if (WEXITSTATUS(status) != 0)
				printf("El comando no se ejecutó correctamente\n");
		return 0;
	}
}
int main(void)
{
    // Los procesos SIGINT Y SIGQUIT solo deben funcionar con los procesos que esten dentro de la shell y no con la propia shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // Se crea un array de caracteres para recibir la linea de comandos pasada por la entrada estandar
    char buffer[1024];

    // Variable encargada de recoger la información tokenizada que se ha introducido
    tline * line;

    int i,existencia;
    printf("msh:%s> ", getcwd(NULL,1024));
    while (fgets(buffer, 1024, stdin))
    {
        existencia = 1;
   
        line = tokenize(buffer);

        // Se comprueba si el usuario a introducido un enter, en ese caso se continuaria la ejecución mostrando el prompt
        if(line->ncommands >=1)
        {
        for (i = 0; i < (line->ncommands); i++)
        {
            if ((line->commands[i].filename == NULL)&&((strcmp(line->commands[i].argv[0], "fg"))&&(strcmp(line->commands[i].argv[0], "cd"))
            &&(strcmp(line->commands[i].argv[0], "jobs"))&&(strcmp(line->commands[i].argv[0], "umask"))&&(strcmp(line->commands[i].argv[0], "exit"))))
            {
                printf("%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
                existencia = 0;
            }
        }
        if (existencia){
            if(!comprobarInternos(line)){
                ejecutarComandoExterno(line);
            }
        }
        }

        printf("msh:%s> ", getcwd(NULL,1024));
    }
    return 0;
}
