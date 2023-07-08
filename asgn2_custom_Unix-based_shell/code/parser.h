#ifndef __PARSER_H
#define __PARSER_H

#include "vector.h"
#define MAX_ARGS 32  // max number of arguments in a command

typedef struct  {
    char *full_cmd;  // entire command as it is typed by the user
    vector_string* args;  // command and its args tokenized
    char * input_file;   // input file name if input redirect
    char * output_file; // output file name if output redirect
    int in_redirect;  // 1 if input redirect, 0 otherwise
    int out_redirect; // 1 if output redirect, 0 otherwise
    int in_fd; // file descriptor for input
    int out_fd; // file descriptor for output)
    int background; // 1 if background, 0 otherwise
}cmd;

cmd *cmd_init(const char *user_input);

void cmd_free(cmd *c);
void str_concat_str(char *s1, char *s2);
void str_concat_char(char *s1, char c);
void cmd_print(cmd *c);
int cmd_parse(cmd *c, char *err);
cmd **tokenise_on_pipe(char *user_input, char *err, int *err_flag, int *num_cmds);

#endif