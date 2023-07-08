#include <stdio.h>
#include <unistd.h>

#include "utils.h"

static void handle_SIGCHLD_block(int condition) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(condition, &mask, NULL);
}

void block_SIGCHLD() {
    handle_SIGCHLD_block(SIG_BLOCK);
}

void unblock_SIGCHLD() {
    handle_SIGCHLD_block(SIG_UNBLOCK);
}

