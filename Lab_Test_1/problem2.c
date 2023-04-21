#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>

void sigint_handler(int sig){
    printf("\nI am unstoppable!\n");
}

int main(){

    char c;

    signal(SIGINT, sigint_handler);

    while(1){
        scanf("%c", &c);
        getchar();
        if (c == 'x'){
            printf("Valar Morghulis\n");
            break;
        }

        if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z')){
            printf("Do u speak my lang?\n");
            continue;
        }
    }
    return 0;
}