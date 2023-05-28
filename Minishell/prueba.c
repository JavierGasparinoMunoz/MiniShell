#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include <errno.h>

// Funcion encargada de comprobar si lo introducido por el usuario es un comando interno de la minishell
int comprobarInternos(tline *line, char* jobsCommands[], pid_t jobsPids[], int *countJobs, mode_t *mascara)
{
    int i, j, f;
    // Se comprueba si el usuario ha introducido el mandato "exit"
    if (!strcmp(line->commands[line->ncommands - 1].argv[0], "exit"))
    {
        if (*countJobs != 0)
        {
            printf("funciona\n");
            for (i = 0; i < *countJobs; i++)
            {
                free(jobsCommands[i]);
            }
            free(jobsPids);
        }
        printf("----Saliendo...----\n");
        exit(0);
        return 1;
    }
    // Se comprueba si el usuario ha introducido el mandato "cd"
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "cd"))
    {
        cd(line);
        return 1;
    }
    // Se comprueba si el usuario ha introducido el mandato "fg"
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "fg"))
    {

        return 1;
    }
    // Se comprueba si el usuario ha introducido el mandato "jobs"
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "jobs"))
    {
        jobs(jobsCommands, jobsPids, countJobs);
        return 1;
    }
    // Se comprueba si el usuario ha introducido el mandato "umask"
    else if (!strcmp(line->commands[line->ncommands - 1].argv[0], "umask"))
    {
        execute_umask(mascara, line);
        return 1;
    }
    return 0;
}

// Función que recoge el código relacionado con el comando "cd"
int cd(tline *line)
{
    char *dir;
    char buffer[512];

    if (line->commands[0].argc > 2)
    {
        fprintf(stderr, "No se pueden introducir más de dos argumentos\n");
        return 1;
    }
    else if (line->commands[0].argc == 1)
    {
        dir = getenv("HOME");
        if (dir == NULL)
        {
            fprintf(stderr, "No existe la variable $HOME\n");
            return 1;
        }
    }
    else
    {
        dir = line->commands[0].argv[1];
    }

    // Comprobar si es un directorio
    if (chdir(dir) != 0)
    {
        fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

void jobs(char *jobsCommands[], pid_t jobsPids[], int *countJobs)
{
    int i;
    printf("Pid     Status      Command \n");
    for (i = 0; i < *countJobs; i++)
    {
        printf("%d      Running     %s \n", jobsPids[i], jobsCommands[i]);
    }
}

int fg()
{
}

int execute_umask(mode_t *mascara, tline *line)
{
    int octal_mask;
    int ceros = 4;
    int aux = *mascara;
    if (line->commands[0].argc > 2)
    {
        fprintf(stderr, "No se pueden introducir más de dos argumentos\n");
        return 1;
    }
    if (line->commands[0].argc == 1)
    {
        printf("Valor de la mascara: %o\n", *mascara);
        return 0;
    }
    else
    {
        octal_mask = strtol(line->commands[0].argv[1], NULL, 8);
        umask(octal_mask);
        printf("Valor de la mascara: %o\n", octal_mask);
        *mascara = octal_mask;
        return 0;
    }
}

int ejecutarComandoExterno(tline *line, char *jobsCommands[], pid_t jobsPids[], int *countJobs)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0)
    { /* Error */
        fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
        return 1;
    }
    else if (pid == 0)
    { /* Proceso Hijo */
        execvp(line->commands[0].filename, line->commands[0].argv);
        // Si llega aquí es que se ha producido un error en el execvp
        printf("Error al ejecutar el comando: %s\n", strerror(errno));
        return 1;
    }
    else
    {
        if (!line->background)
        {
            wait(&status);
            if (WIFEXITED(status) != 0)
            {
                if (WEXITSTATUS(status) != 0)
                {
                    printf("El comando no se ejecutó correctamente\n");
                }
            }
        }
        else
        {
            strcpy(jobsCommands[*countJobs], line->commands[line->ncommands - 1].argv[0]);
            jobsPids[*countJobs] = pid;
            *countJobs = *countJobs + 1;
            printf("%d      %s \n", jobsPids[*countJobs - 1], jobsCommands[*countJobs - 1]);
            waitpid(pid, NULL, 0);
        }
        return 0;
    }
}

int main(void)
{
    // Los procesos SIGINT y SIGQUIT solo deben funcionar con los procesos que estén dentro de la shell y no con la propia shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // Se crea un array de caracteres para recibir la línea de comandos pasada por la entrada estándar
    char buffer[1024];

    // Variable encargada de recoger la información tokenizada que se ha introducido
    tline *line;

    // Variables encargadas de recoger los pids y los nombres de los comandos que estén en background
    char *jobsCommands[30];
    pid_t *jobsPids = (pid_t *)malloc(30 * sizeof(pid_t));

    for (size_t i = 0; i < 30; i++)
    {
        jobsCommands[i] = (char *)malloc(1024 * sizeof(char));
    }

    // Variable encargada de recoger la máscara que se introduzca al comando umask
    mode_t mascara;

    // Se imprime el prompt inicial
    printf("minishell> ");

    // Mientras se siga introduciendo información por la entrada estándar, se procesará dicha información
    while (fgets(buffer, sizeof(buffer), stdin))
    {
        // Se elimina el salto de línea del final de la línea
        buffer[strlen(buffer) - 1] = '\0';

        // Se comprueba si se ha introducido una línea vacía o un comentario (empieza por #)
        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            printf("minishell> ");
            continue;
        }

        // Se crea la estructura de datos tline a partir de la línea introducida por el usuario
        line = tokenize(buffer);

        // Se comprueba si se ha introducido un comando interno de la minishell
        if (comprobarInternos(line, jobsCommands, jobsPids, &countJobs, &mascara) == 1)
        {
            printf("minishell> ");
            continue;
        }

        // Se ejecuta el comando externo
        ejecutarComandoExterno(line, jobsCommands, jobsPids, &countJobs);

        // Se libera la memoria de la estructura tline
        free(line);
        printf("minishell> ");
    }

    // Se libera la memoria de los arrays jobsCommands y jobsPids
    for (size_t i = 0; i < 30; i++)
    {
        free(jobsCommands[i]);
    }
    free(jobsPids);

    return 0;
}
