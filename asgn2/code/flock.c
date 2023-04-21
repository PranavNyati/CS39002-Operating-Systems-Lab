#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/file.h>
#define MAX_CHILD 5

int main()
{
    char *filename = "data.txt";
    FILE *fp = fopen(filename, "w");

    // Apply a shared lock on the file
    int fd = fileno(fp);
    flock(fd, LOCK_SH);

    // Create 5 child processes that will lock the file
    for (int i = 0; i < MAX_CHILD; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // continuously write to the file
            FILE *fp = fopen(filename, "w");
            while (1)
                fprintf(fp, "deth");
            fclose(fp);
        }
    }
    return 0;
}