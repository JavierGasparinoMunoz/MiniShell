#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include <errno.h>
#include <unistd.h>

typedef struct  {
    pid_t pid[100];
    int tamanio;
    char command[100]; //Numero maximo del comando a 100
} Jobs;

// Funcion encargada de comprobar si lo introducido por el usuario es un comando interno de la minishell
int comprobarInternos(tline *line,int numBg, Jobs listaJobs[])
{
    // Se comprueba si el usuario a introducido el mandato exit
    if (!strcmp(line->commands[line->ncommands - 1].argv[0], "exit"))
    {    
        exitExecute(numBg,listaJobs);
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
        if (numBg > 0){
            if (line->commands->argc>1){
                
            } else {
                doFg();
                numBg--;
            }
        } else {
            printf("Mal input en fg");
        }
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato jobs
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "jobs"))
    {
        verJobs();
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato umask
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "umask"))
    {

        return 1;
    }
    return 0;
}

void redireccion(char* in, char* out, char* err){
    FILE *fIn,*fOut,*fErr;
    if (in != NULL){
        fIn = fopen(in,"r");
        if (fIn != NULL){
            dup2(fileno(fIn),STDIN_FILENO);
        } else {
            printf("Error al redireccionar a entrada estandar");
            printf(stderr,"%s:No existe el fichero",in);
            exit(1);
        }
        fclose(fIn);
    }
    if (out != NULL){
        fOut = fopen(out,"w");
        dup2(fileno(fOut),STDOUT_FILENO);      
        fclose(fOut);
    }
    if (err!=NULL){
        fErr = fopen(err,"w");
        dup2(fileno(fErr),STDERR_FILENO);
        fclose(fErr);
    }
}

//Función que recoge el codigo relacionado con el comando cd
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

int jobCount = 0;

void verJobs(Jobs listaJobs[],int *numero)
{
    for (int i = 0; i < (*numero); i++) // comprobamos todas las instrucciones en bg
    {
        for (int j = 0; j < listaJobs[i].tamanio; j++) // comprobamos todos las partes de las instrucciones
        {
            printf("%d      Running     %s \n",i,listaJobs[i].command);
        }
    }
}

void exitExecute(int numBg,Jobs listaJobs[]){
    printf("Saliendo...");
    for (int  i = 0; i < numBg; i++)
    {
        for (int j = 0; j < listaJobs[i].tamanio; j++)
        {
            kill(listaJobs[i].pid[j],9);
        }
    }
    free(listaJobs);
    exit(0);
}
void doFg()
{
    
}

int umask()
{
}

int ejecutarComandoExterno(tline * line, Jobs **listaJobss,int num){
    Jobs *listaJobs = (Jobs *) listaJobss;
    pid_t  pid;
	int status;		
    int pipe1[2],pipe2[2];
    pid_t *pidAux = malloc(line->ncommands);
		
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

    //Variables que contaran el numero de procesos en segundo plano, y variable que los guarda
    int numBg;
    Jobs *listaJobs = malloc(sizeof(Jobs)*100);

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
            if(!comprobarInternos(line,numBg,listaJobs)){
                ejecutarComandoExterno(line,listaJobs,numBg);
            } 
        }
        } else {
            ejecutarComandoExterno(line,listaJobs,numBg);
        }
        

        printf("msh:%s> ", getcwd(NULL,1024));
    }
    return 0;
}
