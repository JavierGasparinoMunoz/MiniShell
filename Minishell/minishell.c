#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

// Funcion encargada de realizar el exit    
void execute_exit(char * jobsCommands[], pid_t * jobsPids[], int * countJobs){
  int j;
  // Si hay procesos en background, se hace kill de los procesos y se libera la memoria de la lista que contiene el nombre de los comandos
  if(*countJobs > 0){
        for(j = 0; j < *countJobs; j++){
            kill(jobsPids[j],SIGKILL);
            free(jobsCommands[j]);
        }
        }
        printf("----Saliendo...----\n");
        exit(0);
}  

//Función que recoge el codigo relacionado con el comando cd
int cd(tline *line)
{
    char *dir;

	
	// Si hay mas de dos argumentos, se muestra un mensaje de error
	if(line->commands[0].argc > 2)
	{
	  fprintf(stderr,"No se pueden introudcir más de dos argumentos\n");
	  return 1;
	}
	
	// Si solo hay un argumento se cambia el directorio al directorio HOME
	else if (line->commands[0].argc == 1)
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
		  fprintf(stderr,"No existe la variable $HOME\n");	
          return 1;
		}
	}
	// Si se ha pasado un argumento, la direccion va a ser el argumento
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

//Funcion encargada de mostrar los procesos que estan en background
void jobs(char * jobsCommands[], pid_t * jobsPids[], int * countJobs)
{
    int i;
    printf("Pid     Status      Command \n");
    for(i = 0; i < *countJobs; i++){
            printf("%d      Running     %s \n",jobsPids[i],jobsCommands[i]);
    }

}

//Funcion que realiza el fg
void fg(char *jobsCommands[], pid_t *jobsPids[], int *countJobs, tline *line) {
    int numero = *countJobs;
    
    // Si hay mas de dos argumentos, se muestra un mensaje de error
	if(line->commands[0].argc > 2)
	{
	  fprintf(stderr,"No se pueden introudcir dos o mas argumentos\n");
	}
    
    // Si no se le pasa un argumento, se pasa a foreground el ultimo proceso que se paso a background
    else if (line->commands[0].argc == 1) {
            printf("%s \n",jobsCommands[*countJobs-1]);
            waitpid(jobsPids[numero - 1], NULL, 0);
        }
        
    // Si se le pasa un argumento se comprueba que el numero que se pasa por argumento si esta dentro de los limites de la lista de los procesos de bg y se pasa a fg    
    else {
        int index = atoi(line->commands[0].argv[1]);
        if(index <= *countJobs){
            printf("%s \n",jobsCommands[index-1]);
            waitpid(jobsPids[index-1], NULL, 0);    
            }
        else{
            fprintf(stderr,"Parametro fuera de los limites\n");
        }    
        

    }
}

// Funcion que recoge el proceso de umask
int execute_umask(mode_t *mascara,tline *line)
{
   // Variable que va a recoger el valor en octal
   int octal_mask = 0;
   
   int ceros = 4;
   int aux = *mascara;
   int aux2;
   
   int esNumero = 1;
   int count = 0;
   char *comprobacionS;
   
   //Si se le pasa mas de dos argumento se muestra un mensaje de error
   if(line->commands[0].argc > 2)
	{
	  fprintf(stderr,"No se pueden introudcir más de dos argumentos\n");
	  return 1;
	}
   // Si no se le pasa ningun argumento, se va a mostrar la mascara en dicho momento. Se rellena con 0 si es necesario	
   if(line->commands[0].argc == 1){
       if (aux == 0){
          ceros-=1;
       }
       while (aux > 0){
          aux /= 10;
          ceros-=1;
       }
       for (; ceros > 0; ceros--){
        printf("0");
       }
       printf("%i\n",*mascara);
      return 0;
   } 
   // Si se le pasa un argumento se cambia a octal y se modifica la mascara por el nuevo valor
   else{
      int longitud = strlen(line->commands[0].argv[1]);
      while((count < longitud) && esNumero){
         if(!isdigit(line->commands[0].argv[1][count])){
           esNumero = 0;
         }
         count+=1;
      }
      if (esNumero){
          octal_mask = strtol(line->commands[0].argv[1],&comprobacionS,8);
          if (*comprobacionS == '\0'){
            umask(octal_mask);
            printf("Valor de la mascara:%o\n",octal_mask);
            *mascara = octal_mask;
            } else {
             fprintf(stderr,"Se debe pasar un numero en octal \n");
             return 1;
            }
      } else {
        fprintf(stderr,"Se debe pasar un numero \n");
        return 1;
      }
     return 0;
   }
}


// Funcion encargada de comprobar si lo introducido por el usuario es un comando interno de la minishell
int comprobarInternos(tline *line, char* jobsCommands[], pid_t * jobsPids[], int * countJobs, mode_t *mascara)
{
    // Se comprueba si el usuario a introducido el mandato exit
    if (!strcmp(line->commands[line->ncommands - 1].argv[0], "exit"))
    {   
        execute_exit(jobsCommands,jobsPids,countJobs);
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
    	// Si hay procesos en background entonces se puede hacer fg
    	if (*countJobs > 0){
    		fg(jobsCommands,jobsPids,countJobs,line);
    	}
    	else{
    	   fprintf(stderr,"No hay procesos en background en estos momentos\n");
    	}
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato jobs
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "jobs"))
    {
        jobs(jobsCommands,jobsPids, countJobs);
        return 1;
    }
    // Se comprueba si el usuario a introducido el mandato umask
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "umask"))
    {
	execute_umask(mascara,line);
        return 1;
    }
    return 0;
}





// Funcion encargada de comprobar si se quiere realizar una redireccion de entrada o salida y la realizacion de la misma. Devuelve 0 si no se quiere redireccionar y 1 si
// se ha redireccionado o ha habido algun error
int redireccion_Ficheros(char * in, char * out,char * err){
   FILE *file;
   // Si se quiere realizar una redireccion de entrada entonces...
   if(in != NULL){
      // Se abre le fichero para lectura
      file = fopen(in, "r");
      // Si el archivo es nulo, se muestra un mensaje de error. Esto ocurre si no se encuentra el archivo
      if(file == NULL){
         fprintf(stderr,"Error al abrir el archivo de entrada\n");
      }
      // Si el archivo existe, se realiza la lectura del archivo
      else{
         dup2(fileno(file),STDIN_FILENO);
      }
      // Se cierra el archivo
      fclose(file);
      return 1;
   }
   // Si se queire realizar una redireccion de salida entonces...
   else if(out !=NULL){
      // Se abre el archivo para escritura
      file = fopen(out,"w");
      // Si al realizar la escritura se genera algun error, se muestra el mensaje de error, sino no se muestra nada y se modifica/crea el archivo con la informacion correspondiente
      if(-1 == dup2(fileno(file),STDOUT_FILENO)){
         fprintf(stderr,"Error al abrir/crear el archivo\n");
      }
      // Se cierra el fichero
      fclose(file);
      return 1;
   }
   //Si se quiere realizar una redirrecion de error entonces...
   else if(err != NULL){
      // Se abre el archivo para escritura
      file = fopen(err,"w");
      // Si al realizar la escritura se genera algun error, se muestra el mensaje de error, sino no se muestra nada y se modifica/crea el archivo con la informacion correspondiente
      if(-1 == dup2(fileno(file),STDERR_FILENO)){
         fprintf(stderr,"Error al abrir/crear el archivo");
      }
      // Se cierra el fichero
      fclose(file);
      return 1;
   }
   return 0;
}


int ejecutarComandoExterno(tline * line, char * jobsCommands[], pid_t * jobsPids[],int * countJobs){
    pid_t  pid;
	int status;		
		
	pid = fork();

	if (pid < 0) { /* Error */
		fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
		return 1;
	}
	else if (pid == 0) { /* Proceso Hijo */
		redireccion_Ficheros(line->redirect_input, line->redirect_output,line->redirect_error);
		execvp(line->commands[0].filename, line->commands[0].argv);
		//Si llega aquí es que se ha producido un error en el execvp
		printf("Error al ejecutar el comando: %s\n", strerror(errno));
		return 1;
	}
	else { 
	// En el caso en el que no se quiera realizar en background
        if(!line->background){
		wait(&status);
		if (WIFEXITED(status) != 0)
			if (WEXITSTATUS(status) != 0)
				printf("El comando no se ejecutó correctamente\n");
        }
        // Si se quiere realizar el mandato en background, se introudce su pid y su nombre en la lista y se suma uno al contador de procesos en bg mostrando la info del mismo
        else{
            strcpy(jobsCommands[*countJobs],line->commands[line->ncommands - 1].argv[0]);
            jobsPids[*countJobs] = pid;
            *countJobs = *countJobs + 1;
            printf("[%d]     %d      %s \n",*countJobs,jobsPids[*countJobs-1],jobsCommands[*countJobs-1]);
        }
	}
   return 0;	
    }
    

// Funcion main
int main(void)
{
    // Los procesos SIGINT Y SIGQUIT solo deben funcionar con los procesos que esten dentro de la shell y no con la propia shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // Se crea un array de caracteres para recibir la linea de comandos pasada por la entrada estandar
    char buffer[1024];

    // Variable encargada de recoger la información tokenizada que se ha introducido
    tline * line;

    // Variables encargadas de recoger los pids y los nombres de los comandos que esten en background
    char * jobsCommands[30];
    
    int i, existencia,countJobs;
    
    for ( size_t i = 0;i < 30; i++){
        jobsCommands[i] = (char *)malloc(1024 * sizeof(char));
    }
    pid_t * jobsPids[30];
    
    for (size_t i = 0;i < 30; i++){
        jobsPids[i] = (pid_t *)malloc(sizeof(pid_t));
    }

    	
    // Variable encargada de recoger la mascara que se posee
    mode_t mascara = 18;
    // Se cambia la mascara
    umask(18);
    
    printf("msh:%s> ", getcwd(NULL,1024));
    //Se inicializa la variable que recoge la cantidad de procesos de background
    countJobs = 0;
    
    while (fgets(buffer, 1024, stdin))
    {
        existencia = 1;
   
        line = tokenize(buffer);
        
        // En este for se comprueba si alguno de los procesos que estaban en bg ha acabado
        for(i=0;i<countJobs;i++){
          if((waitpid(jobsPids[i],NULL,WNOHANG)!=0)){
             if(i!=countJobs-1){
             jobsPids[i] = jobsPids[i+1];
             jobsCommands[i] = jobsCommands[i+1];
             } 
          countJobs = countJobs - 1; 
          } 
        }

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
        //Si existe el mandato que se ha pasado se comprueba si es un mandato interno o es un mandato externo
        if (existencia){
            if(!comprobarInternos(line,&jobsCommands,&jobsPids,&countJobs,&mascara)){
                ejecutarComandoExterno(line,&jobsCommands,&jobsPids,&countJobs);
            }
        }
        }

        printf("msh:%s> ", getcwd(NULL,1024));
    }
    return 0;
}
