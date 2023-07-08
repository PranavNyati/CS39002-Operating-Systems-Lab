#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#include "parser.h"

void handle_io_redirect(cmd *c){

    if (c->in_redirect == 1){

        if ( (c->in_fd = open(c->input_file, O_RDONLY)) == -1){
            printf("Error opening input file: %s\n", c->input_file);
            exit(1);
        }
    }

    if (c->out_redirect == 1){
        c->out_fd = open(c->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    return;
}