#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>


static void handle_SIGCHLD_block(int sig);
void block_SIGCHLD();
void unblock_SIGCHLD();


#endif