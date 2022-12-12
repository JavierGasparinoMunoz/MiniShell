#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"

// Funcion encargada de comprobar si lo introducido por el usuario es un comando interno de la minishell
int comprobarInternos(tline *line)
{
    int i;
    // Se comprueba si el usuario a introducido el mandato exit
    if (!strcmp(line->commands[line->ncommands - 1].argv[0], "exit"))
    {
        printf("----Saliendo...----");
        exit(0);
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato cd
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "cd"))
    {
        cd();
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato fg
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "fg"))
    {
        fg();
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato jobs
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "jobs"))
    {
        jobs();
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato umask
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "umask"))
    {
        umask();
        return 1;
    }
    return 0;
}

//Funcion encargada de ver si el comando introducido por el usuario se corresponde a un comando externo
int comprobarExternos(){

}

int cd(tline *line)
{
    char *dir;
	char buffer[512];
	
	if(line->commands[line->ncommands - 1].argc > 2)
	{
	  fprintf(stderr,"Uso: %s directorio\n", argv[0]);
	  return 1;
	}
	
	else if (line->commands[line->ncommands - 1].argc == 1)
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
		  fprintf(stderr,"No existe la variable $HOME\n");	
		}
	}
	else 
	{
		dir = argv[1];
	}
	
	// Comprobar si es un directorio
	if (chdir(dir) != 0) {
			fprintf(stderr,"Error al cambiar de directorio: %s\n", strerror(errno));  
	}
	printf( "El directorio actual es: %s\n", getcwd(buffer,-1));

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

int main(void)
{
    // Los procesos SIGINT Y SIGQUIT solo deben funcionar con los procesos que esten dentro de la shell y no con la propia shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // Se crea un array de caracteres para recibir la linea de comandos pasada por la entrada estandar
    char buf[1024];

    // Variable encargada de recoger la información tokenizada que se ha introducido
    tline *line;

    int i, j,existencia;
    printf("msh:%s> ", getcwd(NULL,1024));
    while (fgets(buf, 1024, stdin))
    {
        existencia = 0;
   
        line = tokenize(buf);

        // Se comprueba si el usuario a introducido un enter, en ese caso se continuaria la ejecución mostrando el prompt
        if (line == NULL)
        {
            continue;
        }
        else
        {
        for (i = 0; i < (line->ncommands - 1); i++)
        {
            if (line->commands[i].filename == NULL)
            {
                printf("%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
                existencia = 1;
            }
        }
        if (existencia){
            comprobarInternos(line);
            if (comprobarInternos(line)){
                comprobarExternos(line);
            }
        }
        }

        printf("msh:%s> ", getcwd(NULL,1024));
    }
    return 0;
}
