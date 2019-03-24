#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define READ 0
#define WRITE 1
#define BUFFER_SIZE 512

/** @brief Bitmask to hold running options.
 *      Bit 5 | Bit 4 | Bit 3 | Bit 2  | Bit 1 | Bit 0
 *       -r      md5     sha1   sha256    -o      -v
*/
unsigned int mask = 0;
char outfile[25];

/** @brief ... */
void init(int argc, char *argv[])
{

    char *token;

    if (argc < 2)
    {
        printf("Usage: %s <file|dir>\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc - 1; i++)
    {
        if (!strcmp(argv[i], "-r"))
            mask |= 0x20;

        if (!strcmp(argv[i - 1], "-h"))
        {
            token = strtok(argv[i], ",");

            while (token != NULL)
            {

                if (!strcmp(token, "md5"))
                    mask |= 0x10;

                if (!strcmp(token, "sha1"))
                    mask |= 0x08;

                if (!strcmp(token, "sha256"))
                    mask |= 0x04;

                token = strtok(NULL, ",");
            }
        }

        if (!strcmp(argv[i - 1], "-o"))
        {
            strcpy(outfile, argv[i]);
            mask |= 0x02;
        }

        if (!strcmp(argv[i], "-v"))
            mask |= 0x01;
    }
}

/** @brief returns a string with the date complying with the ISO 8601 format, <date>T<time> */
char *formatDate(char *s, time_t val)
{
    strftime(s, 50, "%Y-%m-%dT%X", localtime(&val));
    return s;
}

/** @brief runs a given shell command in the format "command filename" */
void runCommand(char command[], char filename[])
{

    char buffer[BUFFER_SIZE];
    char *token;
    int pipe_fd[2]; //pipe used for redirecting the output of the command
    pid_t pid;

    /* setting the communication pipe */
    if (pipe(pipe_fd) < 0)
    {
        perror("runCommand: pipe");
        exit(1);
    }

    /* runs the command */
    pid = fork();

    if (pid != 0) // PARENT
    {
        close(pipe_fd[WRITE]);
        wait(NULL); /* wait for the child to terminate */
    }
    else if (pid == 0) // CHILD
    {
        close(pipe_fd[READ]);
        dup2(pipe_fd[WRITE], STDOUT_FILENO);
        execlp(command, command, filename, NULL);
        perror("runCommand");
        exit(2);
    }

    /* prints the command output */
    read(pipe_fd[READ], buffer, BUFFER_SIZE);
    if (!strcmp(command, "file"))
    {
        /* printing first two fields of file command*/
        token = strtok(buffer, ":");
        printf("%s", token);
        token = strtok(NULL, ",\n");
        printf(",%s", ++token);
    }
    else
        printf(",%s", strtok(buffer, " ")); /* only prints the argument before the ' ' (space) */
}

/** @brief prints the stats of a file */
void printStats(struct stat fileStat, char filename[])
{

    char date[50];

    runCommand("file", filename);                         /* file name and type */
    printf(",%lu", fileStat.st_size);                     /* file size */
    printf(",%x", fileStat.st_mode);                      /* file protection */
    printf(",%s", formatDate(date, fileStat.st_atime));   /* time of last access */
    printf(",%s", formatDate(date, fileStat.st_mtime)); /* time of last modification */

    /* md5 */
    //If bit is set, numerical value will be greater than 0 -> true
    if (mask & 0x10)
        runCommand("md5sum", filename);

    /* sha1 */
    if (mask & 0x08)
        runCommand("sha1sum", filename);

    /* sha256 */
    if (mask & 0x04)
        runCommand("sha256sum", filename);

    printf("\n");
}

int main(int argc, char *argv[], char *envp[])
{

    struct stat fileStat;

    init(argc, argv);

    if (stat(argv[argc - 1], &fileStat))
    {
        perror("Stat");
        exit(1);
    }

    printStats(fileStat, argv[argc - 1]);

    return 0;
}