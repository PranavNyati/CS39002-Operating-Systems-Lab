#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

#include "parser.h"

cmd *cmd_init(const char *user_input){
    cmd *c = (cmd *)malloc(sizeof(cmd));
    c->full_cmd = (char *)malloc(sizeof(char) * (strlen(user_input) + 1));
    strcpy(c->full_cmd, user_input);
    c->args = vector_string_init(MAX_ARGS);
    c->input_file = NULL;
    c->output_file = NULL;
    c->in_redirect = 0;
    c->out_redirect = 0;
    c->in_fd = 0;
    c->out_fd = 1;
    c->background = 0;

    return c;
}

void cmd_free(cmd *c){
    vector_string_free(c->args);
    free(c->full_cmd);
    free(c);
}

void str_concat_str(char *s1, char *s2){
    s1 = (char *)realloc(s1, sizeof(char) * (strlen(s1) + strlen(s2) + 1));
    strcat(s1, s2);
    s1[strlen(s1) + 1] = '\0';
}

void str_concat_char(char *s1, char c){
    s1 = (char *)realloc(s1, sizeof(char) * (strlen(s1) + 2));
    int p =strlen(s1);
    s1[p] = c;
    s1[p + 1] = '\0';
}

int cmd_parse(cmd *c, char *err){
    vector_string *tokens = vector_string_init(MAX_ARGS);
    char *temp = strdup("");

    for (int i = 0; i < strlen(c->full_cmd); i++){

        if (c->full_cmd[i] == '\\'){

            i++;
            if (i < strlen(c->full_cmd) ){
                str_concat_char(temp, c->full_cmd[i]);
                i++;
            }
            else{
                strcpy(err, "Invalid command! Command cannot end with a backslash\n");
                return -1;
            }
        }

        if (c->full_cmd[i] == '<' || c->full_cmd[i] == '>' || c->full_cmd[i] == '&'){

            if (strlen(temp) != 0){
                vector_string_push_back(tokens, temp);
            }
            temp = strdup("");
            str_concat_char(temp, c->full_cmd[i]);
            vector_string_push_back(tokens, temp);
            temp = strdup("");

        }

        else if (c->full_cmd[i] == '"'){

            if (i!= 0 && c->full_cmd[i-1] == '/'){
                str_concat_char(temp, c->full_cmd[i]);
                i++;
                while(i < strlen(c->full_cmd) && (c->full_cmd[i] != '"' || (c->full_cmd[i] == '"' && c->full_cmd[i-1] == '\\'))){
                    str_concat_char(temp, c->full_cmd[i]);
                    i++;
                }
                if (i == strlen(c->full_cmd) ){
                    strcpy(err, "Invalid command! Error in handling quotes\n");
                    return -1;
                }
                if (c->full_cmd[i] == '"')
                    str_concat_char(temp, c->full_cmd[i]);
                
            }

            else{
                i++;
                while(i < strlen(c->full_cmd)  && (c->full_cmd[i] != '"' || (c->full_cmd[i] == '"' && c->full_cmd[i-1] == '\\'))){
                    str_concat_char(temp, c->full_cmd[i]);
                    i++;
                }
                if (i == strlen(c->full_cmd)){
                    strcpy(err, "Invalid command! Error in handling quotes\n");
                    return -1;
                }
            }
        }

        else if (c->full_cmd[i] == ' '){
            if (strlen(temp) != 0){
                vector_string_push_back(tokens, temp);
                temp = strdup("");
            }
            else 
                continue;
        }

        else{
            str_concat_char(temp, c->full_cmd[i]);
        }
    }

    if (strlen(temp) != 0){
        vector_string_push_back(tokens, temp);
        temp = strdup("");
    }

    // for (int i = 0; i < tokens->size; i++){
    //     for (int j = 0; j < strlen(tokens->data[i]); j++){
    //         printf("%d_", tokens->data[i][j]);
    //     }
    //     printf("\n");

    // }

    for (int j = 0; j < tokens->size; j++){
        if (!strcmp(tokens->data[j], "<")){
            if (j == tokens->size - 1 || !strcmp(tokens->data[j+1], ">") || !strcmp(tokens->data[j+1], "<")){
                strcpy(err, "Invalid command! Incorrect use of input redirection\n");
                return -1;
            }
            c->input_file = strdup(tokens->data[j+1]);
            c->in_redirect = 1;
            j+=1;
        }

        else if (!strcmp(tokens->data[j], ">")){
            if (j == tokens->size - 1 || !strcmp(tokens->data[j+1], ">") || !strcmp(tokens->data[j+1], "<")){
                strcpy(err, "Invalid command! Incorrect use of input redirection\n");
                return -1;
            }
            c->output_file = strdup(tokens->data[j+1]);
            c->out_redirect = 1;
            j+=1;
        }

        else if (!strcmp(tokens->data[j], "&")){
            if (j != tokens->size - 1){
                strcpy(err, "Invalid command! No other arguments allowed after &\n");
                return -1;
            }
            c->background = 1;
        }

        else{

            // if jth token contains * or ? then it is a wildcard
            if ( (strchr(tokens->data[j], '*') != NULL ) || (strchr(tokens->data[j], '?') != NULL) )
            {
                // invoke the glob function on this token
                glob_t wildcard_matches;
                // printf("globbing %s\n", tokens->data[j]);
               
                int ret_val =  glob(tokens->data[j], 0, NULL, &wildcard_matches);

                if (ret_val != 0){   // in case of error in globbing, handle different error cases

                    switch (ret_val){
                        case GLOB_NOSPACE:
                            strcpy(err, "glob failed: insufficient memory\n");
                            break;
                        
                        case GLOB_ABORTED:
                            strcpy(err, "glob failed: read error\n");
                            break;
                        
                        case GLOB_NOMATCH:
                            strcpy(err, "glob failed: no match found\n");
                            break;

                        default:
                            strcpy(err, "glob failed: unknown error\n");
                            break;
                    }

                    globfree(&wildcard_matches);
                    return -1;
                }

                else {
                    // printf("globbing successful\n");
                    // printf("number of glob matches: %ld\n", wildcard_matches.gl_pathc);
                    for (int k = 0; k < wildcard_matches.gl_pathc; k++){
                        // printf("%s\n", wildcard_matches.gl_pathv[i]);
                        vector_string_push_back(c->args, wildcard_matches.gl_pathv[k]);
                    }

                    globfree(&wildcard_matches);
                }
                

            }

            else {
                vector_string_push_back(c->args, tokens->data[j]);
            }
        }
    }

    return 0;

}


void cmd_print(cmd *c){
    printf("full_cmd: %s\n", c->full_cmd);
    printf("args:\n");
    for (int i = 0; i < c->args->size; i++){
        printf("arg[%d]: %s\n", i, c->args->data[i]);
    }
    printf("\n");
    printf("input_file: %s\n", c->input_file);
    printf("output_file: %s\n", c->output_file);
    printf("in_redirect: %d\n", c->in_redirect);
    printf("out_redirect: %d\n", c->out_redirect);
    printf("background: %d\n", c->background);
    return;
}


cmd **tokenise_on_pipe(char *user_input, char *err, int *err_flag, int *num_cmds)
{
    vector_string *piped_cmds = vector_string_init(MAX_ARGS);
    char *temp = strdup("");

    for (int i = 0; i < strlen(user_input) - 1; i++)
    {
        if (user_input[i] == '"')
        {
            str_concat_char(temp, user_input[i]);
            i++;
            while (i < strlen(user_input) - 1 && (user_input[i] != '"' || (user_input[i] == '"' && user_input[i - 1] == '\\')))
            {
                str_concat_char(temp, user_input[i]);
                i++;
            }
            if (i == strlen(user_input) - 1)
            {
                strcpy(err, "Invalid command! Error in quotes");
                *err_flag = -1;
                return NULL;
            }
            if (user_input[i] == '"')
            {
                str_concat_char(temp, user_input[i]);
            }
        }

        else if (user_input[i] == '|')
        {
            if (i == strlen(user_input) - 2)
            { // the user has entered a command like "ls | wc |"
                strcpy(err, "Invalid command! No command after a pipe.\n");
                *err_flag = -1;
                return NULL;
            }

            if (strlen(temp) != 0)
            {
                vector_string_push_back(piped_cmds, temp);
                temp = strdup("");

                while (user_input[i + 1] == ' ' && i < strlen(user_input) - 1)
                { // remove the unnnecesarry spaces after pipe
                    i++;
                }
                if (i == strlen(user_input))
                { // the user has entered a command like "ls | wc |   "
                    strcpy(err, "Invalid command! No command after a pipe.\n");
                    *err_flag = -1;
                    return NULL;
                }
            }

            else
            { // the user has entered a command like "ls || wc" or the cmd starts with a pipe
                strcpy(err, "Invalid command! Error in using pipe.\n");
                *err_flag = -1;
                return NULL;
            }
        }

        else
        {
            str_concat_char(temp, user_input[i]);
        }
    }

    if (strlen(temp) != 0)
    {
        vector_string_push_back(piped_cmds, temp);
    }

    cmd **piped_cmds_seq = (cmd **)malloc(piped_cmds->size * sizeof(cmd *));
    int bg_flag = 0;


    // for (int i = 0; i < piped_cmds->size; i++)
    // {
    //     printf("cmd %d: %s\n", i, piped_cmds->data[i]);
    // }

    // check if the & is present in the last command in the seq of piped commands
    for (int i = 0; i < piped_cmds->size; i++)
    {
        // if the & is not present in the last command in the seq of piped commands
        // printf("%s\n", piped_cmds->data[i]);
        if (strchr(piped_cmds->data[i], '&') != NULL)
        {
            if (i != piped_cmds->size - 1)
            {
                strcpy(err, "Invalid command! Cannot have an & in a cmd if it is not the last cmd in a seq of piped cmds.\n");
                *err_flag = -1;
                return NULL;
            }
            else
            {
                bg_flag = 1;
            }
        }
    }

    *num_cmds = piped_cmds->size;
    // printf("Num commands: %d\n", *num_cmds);

    for (int i = 0; i < piped_cmds->size; i++)
    {
        piped_cmds_seq[i] = cmd_init(piped_cmds->data[i]);
        if (bg_flag == 1 && i == piped_cmds->size - 1)
        {
            piped_cmds_seq[i]->background = 1;
        }
    }

    return piped_cmds_seq;
}


    