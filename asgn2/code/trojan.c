#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_PAR_CHILD 2
#define MAX_CHILD_CHILD 2
#define SLEEP_TIME 10

void attack()
{
    while (1)
        __asm("");
}

int main()
{
    printf("root pid: %d", getpid());
    sleep(SLEEP_TIME);
    for (int i = 0; i < MAX_PAR_CHILD; i++)
    {
        pid_t p = fork();
        if (p == 0)
        {
            printf("trojan child: %d", getpid());
            for (int i = 0; i < MAX_CHILD_CHILD; i++)
            {
                pid_t pid = fork();
                if (pid == 0)
                {
                    printf("trojan grandchild: %d", getpid());
                    attack();
                }
            }
            attack();
        }
    }
    sleep(120);
    return 0;
}
