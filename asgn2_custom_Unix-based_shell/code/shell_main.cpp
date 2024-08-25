#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <set>
#include <vector>
#include <iostream>
#include <signal.h>
#include <stack>
#include <fcntl.h>

#include "parser.h"
#include "editing.h"
#include "utils.h"
#include "io_redirect.h"
#include "sb.h"

#define MAX_BUFF_SIZE 1024

using namespace std;

int curr_pg_id = 0;
set <int> fg_processes_pid;  // set of foreground processes' pid
set <int> bg_processes_pid;  // set of background processes' pid
stack <int> suspended_proc_pid;  // stack of suspended processes' pid

int send_fg_to_bg = 0;
int fg_process_group_id = 0;

// fpid is the pid of the foreground process if there is no pipe, else it is the pid of the last process in the pipe
static pid_t fgpid = 0; // 0 means no foreground process 

static void manage_child(int sig){
    int status;
    pid_t ret_pid;
    while(true){
        ret_pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (ret_pid == 0){  // there are one or more child processes in execution, but none of them have exited 
            break;
        }

        else if (ret_pid == -1){  // there are no child processes in execution, or there was an error
            // perror("waitpid");
            break;
        }

        else{
            if (ret_pid == fgpid){ // if the child process that exited was the foreground process
                fgpid = 0;  // set the fgpid to 0 to denote that no foreground process is running from now on
            }
            
            if (fg_processes_pid.find(ret_pid) != fg_processes_pid.end()){  // if the process that exited was a foreground process
                fg_processes_pid.erase(ret_pid);  // remove the pid of the foreground process from the set of foreground processes
                
                if (WIFSTOPPED(status)){
                    // printf("\nCtrl-Z pressed.\n");
                    printf("\nFg process with pid %d moved to background with status %d\n", ret_pid, WSTOPSIG(status));
                    suspended_proc_pid.push(ret_pid);
                }


                else if (WIFEXITED(status)){
                    printf("\nFg process with pid %d exited normally with status %d\n", ret_pid, WEXITSTATUS(status));
                }

                else{
                    printf("\nFg process with pid %d exited abnormally with status %d\n", ret_pid, WEXITSTATUS(status));    
                }
            
            }

            if (bg_processes_pid.find(ret_pid) != bg_processes_pid.end()){  // if the process that exited was a background process
                bg_processes_pid.erase(ret_pid);  // remove the pid of the background process from the set of background processes
            
                if(WIFEXITED(status)){
                    printf("\nBg process with pid %d exited normally with status %d\n", ret_pid, WEXITSTATUS(status));
                }
                else{
                    printf("\nBg process with pid %d exited abnormally with status %d\n", ret_pid, WEXITSTATUS(status));    
                }
            }

        }

    }
    return;
}

// This function is called when the user presses Ctrl-C as a Ctrl-C signal is sent to the foreground process group
void sigint_handler(int sig){
    return;
}

// This function is called when the user presses Ctrl-Z as a Ctrl-Z signal is sent to the foreground process group
void sigtstp_handler(int sig){
    return;
}

// this function waits for a foreground process to finish execution
static void wait_for_fg_process(pid_t pid) {
    fgpid = pid;
    sigset_t empty; // empty set of signals
    sigemptyset(&empty);   // initialize the empty set of signals 
    while (fgpid == pid) {

        // When some unblocked signal arrives, the process gets the CPU, the signal is handled, the original blocked
        // set is restored, and sigsuspend returns.
        sigsuspend(&empty);   
    }
    unblock_SIGCHLD();
}

// this function is responsible for executing a command
void handle_process(cmd ** cmd_seq, int *num_piped_cmds, int background){
    
    curr_pg_id = 0;
    fg_process_group_id = 0;
    //clear the set of foreground processes, if any
    fg_processes_pid.clear();
    send_fg_to_bg = 0;

    while (suspended_proc_pid.size() > 0){
        suspended_proc_pid.pop();
    }

    int pipefd[2];   // initialize the pipe file descriptors
    int last_cmd_pid = 0;
    memset(pipefd, 0, sizeof(pipefd));
    // printf("No of cmds: %d\n", *num_piped_cmds);

    for (int i = 0; i < *num_piped_cmds; i++){

        // printf("%d command in process:\n", i);

        cmd_seq[i]->in_fd = STDIN_FILENO;  // set the input file descriptor of a command to STDIN_FILENO
        cmd_seq[i]->out_fd = STDOUT_FILENO; // set the output file descriptor of a command to STDOUT_FILENO
        
        handle_io_redirect(cmd_seq[i]);  // opens the i/o redirection files, if any and sets the file descriptors in the cmd struct
       

        if (i > 0){  // for all other commands except 1st one in pipe , set the input file descriptor to the read end of the pipe
                     // for 1st command, its input file descriptor is already handled by the handle_io_redirect function
            cmd_seq[i]->in_fd = pipefd[0];  
        }

        if (i < *num_piped_cmds -1 ){

            if (pipe(pipefd) == -1){
                perror("pipe: ");
                exit(1);
            }

            cmd_seq[i]->out_fd = pipefd[1];  // set the output file descriptor of a command to the write end of the pipe
        }

        block_SIGCHLD();  // block the SIGCHLD signal, so that the child process does not get killed before the parent process has a chance to add it to the set of foreground processes
        
        pid_t child_pid = fork();

        if (child_pid == -1){
            perror("Failed to fork.\n");
            exit(1);
        }

        if (i == *num_piped_cmds - 1){  // if the command is the last command in the pipe
            last_cmd_pid = child_pid;  // set the last_cmd_pid to the pid of the last command in the pipe
        }

        if (child_pid == 0){  // code segment for the child process

            unblock_SIGCHLD();  // unblock the SIGCHLD signal in the child process as it may also fork its own child processes and we want to be able to handle the SIGCHLD signal in the child process

            // for i == 0, set the process group id of the 1st child process to its own pid 
            // set the process group id of the 1st child process to its own pid 
            // (i.e. the 1st child process in a piped set of cmds 
            // becomes the leader of its own process group, and 
            // the other child processes in the piped set of cmds 
            // become the members of the process group of the 1st child process)
            
            // for i > 0, set the process group id of the child process to the process group id of the 1st child process

            if (setpgid(0, curr_pg_id) == -1){
                perror("setpgid: ");
                exit(1);
            }


            if (cmd_seq[i]->in_fd != STDIN_FILENO){  // if the input file descriptor of the command is not STDIN_FILENO, then close the STDIN_FILENO file descriptor
                dup2(cmd_seq[i]->in_fd, STDIN_FILENO);  // duplicate the input file descriptor of the command to the STDIN_FILENO file descriptor
                close(cmd_seq[i]->in_fd);
            }

            if (cmd_seq[i]->out_fd != STDOUT_FILENO){  // if the output file descriptor of the command is not STDOUT_FILENO, then close the STDOUT_FILENO file descriptor
                dup2(cmd_seq[i]->out_fd, STDOUT_FILENO);  // duplicate the output file descriptor of the command to the STDOUT_FILENO file descriptor
                close(cmd_seq[i]->out_fd);
            }

            // printf("Executing command: %s\n", cmd_seq[i]->args->data[0]);
            if (execvp(cmd_seq[i]->args->data[0], cmd_seq[i]->args->data) == -1){
                perror("Failed to execute command.\n");
                exit(1);
            }
        }

        // setpgid also  called in the parent as well as the child process as we don't know which process will execute first
        setpgid(child_pid, curr_pg_id);  // set the process group id of the child process to the process group id of the 1st child process 
        if (curr_pg_id == 0){

            // curr_pg_id is given the value of the pid of the 1st child process 
            // so that the other child processes in the piped set of cmds 
            // can be added to the process group of the 1st child process 
            curr_pg_id = child_pid;  
            fg_process_group_id = child_pid;  // set the process group id of the foreground process group to the process group id of the 1st child process

            if (background == 0){  // for the process grp with curr_pg_id (this process is assumed to start in fg), set it as the foreground process grp on the terminal associated with the stdin file descriptor
                tcsetpgrp(STDIN_FILENO, curr_pg_id); // this will block a background process from accessing STDIN of terminal until the foreground process finishes
            }

        }


        if (background == 0)  // if the command is not a background process, add it to the set of foreground processes
            fg_processes_pid.insert(child_pid);
        
        else
            bg_processes_pid.insert(child_pid);
        
        unblock_SIGCHLD();  // unblock the SIGCHILD signal now, so that the parent process can handle the SIGCHLD signal

        if (i < *num_piped_cmds -1 ){  // close the read end of the pipe, if it is not the last command in the pipe
            close(cmd_seq[i]->out_fd);
        }

    }

    if (background == 1)
    {  // if the command is a background process, print the pid of the last command in the pipe
        unblock_SIGCHLD();
    }

    else if (background == 0){  // if the command is not a background process, wait for the last command in the pipe to finish executing
        // printf("Waiting for process %d to finish executing.\n", last_cmd_pid);
        wait_for_fg_process(last_cmd_pid);
    
        // continue any suspended process in background
        while (!suspended_proc_pid.empty()){
            bg_processes_pid.insert(suspended_proc_pid.top());
            kill(suspended_proc_pid.top(), SIGCONT);
            suspended_proc_pid.pop();
        }

        // reset the process grp of the parent(our own shell) as the foreground process grp on the terminal associated with the stdin file descriptor
        if (tcsetpgrp(STDIN_FILENO, getpgid(0)) == -1){
            perror("tcsetpgrp: ");
            exit(1);
        }

    }

    // clear the set of foreground processes, if any
    fg_processes_pid.clear();

    while (!suspended_proc_pid.empty()){
        suspended_proc_pid.pop();
    }

    curr_pg_id = 0;  // reset the curr_pg_id to 0
    fg_process_group_id = 0;  // reset the fg_process_group_id to 0
    send_fg_to_bg = 0;  // reset the send_fg_to_bg flag to 0
}



int main()
{

    pid_t child_pid, ret_pid;
    // int status;

    fg_processes_pid.clear();
    bg_processes_pid.clear();

    // setting the signal handlers     
    signal(SIGCHLD, manage_child); // SIGCHLD is sent to the parent process when a child process terminates either normally or abnormally

    signal(SIGINT, sigint_handler); // SIGINT is sent to the process when the user presses Ctrl+C

    signal(SIGTSTP, sigtstp_handler); // SIGTSTP is sent to the process when the user presses Ctrl+Z, but here we need to send process to the background if Ctrl+Z is pressed

    signal(SIGTTOU, SIG_IGN); 

    while(1){
        printf("\033[1;31m");  // set the text color to red
        printf("%s:", getenv("USER"));
        printf("\033[1;33m");            // set the text color to yellow
        printf("%s$ ", getcwd(NULL, 0)); // getcwd() returns the current working directory (char *
        printf("\033[0m");

        size_t input_size = 0;
        char *user_input = NULL;
   
        // // read the user input
        user_input = (char *)malloc(MAX_BUFF_SIZE * sizeof(char));

        if (fgets(user_input,MAX_BUFF_SIZE,stdin)==NULL)
        {
            free(user_input);
            perror("Failed to read input.\n");
            exit(1);
        }
        input_size = strlen(user_input);
        // printf("%s", user_input);
    
        // user_input = get_cmd();
        // input_size = strlen(user_input);

        // tokenize the user input on the basis of the pipe character
        char *err = strdup("");
        int err_flag = 0;
        int num_pipe_cmds = 0;
        int background_flag = 0;

        cmd **piped_cmds_seq = tokenise_on_pipe(user_input, err, &err_flag, &num_pipe_cmds);
        free(user_input);

        // if the user has entered an invalid command
        if (err_flag == -1)
        {
            if (piped_cmds_seq != NULL)
            {
                for (int i = 0; i < num_pipe_cmds; i++)
                {
                    cmd_free(piped_cmds_seq[i]);
                }
                free(piped_cmds_seq);
            }
            printf("%s\n", err);
            continue;
        }
        err = strdup("");

        for (int i = 0; i < num_pipe_cmds; i++)
        {

            // tokenise the individual commands
            if (cmd_parse(piped_cmds_seq[i], err) == -1)
            {
                printf("%s\n", err);
                err_flag = -1;
                break;
            }
            if (piped_cmds_seq[i]->background == 1){
                background_flag = 1;
            }
        }

        if (err_flag == -1)
        { // atleast one of the subcommands is invalid
            continue;
        }

        // for (int i = 0; i < num_pipe_cmds; i++)
        // {
        //     // printf("cmd no: %d\n", i);
        //     cmd_print(piped_cmds_seq[i]);
        //     printf("\n\n");
        // }


        if (num_pipe_cmds == 0){
            continue;
        }

        // in case exit command is entered, to be run in the parent process
        if (!strcmp(piped_cmds_seq[0]->args->data[0], "exit"))
        {

            if (piped_cmds_seq[0]->args->size > 2)
            { // more than 1 argument in the exit command
                printf("Invalid command! Only 1 numeric arg (exit status) allowed with the exit command.\n");
                continue;
            }
            else
            {

                if (piped_cmds_seq[0]->args->size == 2)
                { // the user has entered an exit status
                    int exit_status = atoi(piped_cmds_seq[0]->args->data[1]);
                    printf("Exiting the shell with exit status %d.\n", exit_status);
                    exit(exit_status);
                }
                else
                { // the user has not entered an exit status
                    printf("Exiting the shell.\n");
                    exit(0);
                }
            }
        }

        // if cd is entered, it has to be run in the parent process
        else if (!strcmp(piped_cmds_seq[0]->args->data[0], "cd"))
        {
            if (piped_cmds_seq[0]->args->size > 2)
            { // more than 1 argument in the cd command
                printf("cd: Too many arguments.\n");
                continue;
            }
            else
            {
                if (piped_cmds_seq[0]->args->size == 2)
                { // the user has not entered a path
                    if (!strcmp(piped_cmds_seq[0]->args->data[1], "~"))
                    { // the user has entered ~ as the path
                        chdir(getenv("HOME"));
                    }
                    else if (!strcmp(piped_cmds_seq[0]->args->data[1], "-"))
                    { // the user has entered - as the path
                        chdir(getenv("OLDPWD"));
                    }
                    else
                    { // the user has entered a path
                        if (chdir(piped_cmds_seq[0]->args->data[1]) == -1)
                        {
                            perror("cd: ");
                        }
                    }
                }

                else if (piped_cmds_seq[0]->args->size == 1)
                { // the user has not entered a path
                    chdir(getenv("HOME"));
                }
            }
        }

        // delep command 
        else if (!strcmp(piped_cmds_seq[0]->args->data[0], "delep"))
        {
            if (piped_cmds_seq[0]->args->size > 2)
            { // more than 1 argument in the cd command
                printf("delep: Too many arguments.\n");
                continue;
            }
            else
            {
                int fd[2];
                pipe(fd);
                if (fcntl(fd[0], F_SETFL, O_NONBLOCK) < 0)
                    exit(2);
                pid_t p = fork();
                if (p > 0)
                {
                    wait(NULL);
                    close(fd[1]);
                    dup(fd[0]);
                    int *pids = (int *)malloc(100 * sizeof(int));
                    for (int i = 0; i < 100; i++)
                        pids[i] = -1;

                    char buffer[1024];

                    while (read(fd[0], buffer, sizeof(buffer)) != 0)
                    {
                        char *token = strtok(buffer, " ");
                        int i = 0;
                        while (token != NULL)
                        {
                            pids[i] = atoi(token);
                            token = strtok(NULL, " ");
                            i++;
                        }
                    }

                    // print pids
                    cout << "processes locking file:\n";
                    for (int i = 0; pids[i] > 0; i++)
                        cout << pids[i] << endl;
                    if (pids[0] == -1)
                    {
                        cout << "no processes found\n";
                        try
                        {
                            remove(piped_cmds_seq[0]->args->data[1]);
                        }
                        catch (...)
                        {
                            cout << "error removing file" << endl;
                        }
                        continue;
                    }
                    cout << "kill? (y/n):\n";
                    char c = 'f';
                    while (c != 'y' && c != 'n')
                    {
                        cin >> c;
                        if (c == 'y')
                        {
                            for (int i = 0; pids[i] > 0; i++)
                                kill(pids[i], SIGKILL);
                            try
                            {
                                remove(piped_cmds_seq[0]->args->data[1]);
                            }
                            catch (...)
                            {
                                cout << "error removing file" << endl;
                            }
                        }
                        else
                            cout << "kill? (y/n):\n";
                    }
                }
                else if (p == 0)
                {
                    close(1);
                    close(fd[0]);
                    dup(fd[1]);
                    if (execlp("fuser", "fuser", piped_cmds_seq[0]->args->data[1], NULL) == -1)
                    {
                        perror("fuser error");
                        continue;
                    }
                }
                else
                {
                    perror("fork error");
                    continue;
                }
            }
        }

        else if (!strcmp(piped_cmds_seq[0]->args->data[0], "sb"))
        {
            cout << "sb\n";
            if (piped_cmds_seq[0]->args->size < 2)
            {
                cout << "Usage: " << piped_cmds_seq[0]->args->data[0] << " <pid> [-suggest]" << endl;
                cout << "Pass -suggest to kill all processes in the bug's process group\n";
                continue;
            }
            int pid;
            int suggest = 0;
            try
            {
                pid = stoi(piped_cmds_seq[0]->args->data[1]);
                if (piped_cmds_seq[0]->args->size == 3 && strcmp(piped_cmds_seq[0]->args->data[2], "-suggest") == 0)
                    suggest = 1;
                else if (piped_cmds_seq[0]->args->size == 3 && strcmp(piped_cmds_seq[0]->args->data[2], "-suggest") != 0)
                {
                    cout << "invalid argument\n";
                    continue;
                }
            }
            catch (...)
            {
                cout << "invalid pid: " << piped_cmds_seq[0]->args->data[1] << endl;
                continue;
            }
            // create pipe for communication in parent-child
            int fd[2];
            pipe(fd);

            // fork child
            pid_t p = fork();

            if (p > 0)
            {
                // parent process
                wait(NULL);
                close(fd[1]);
                dup(fd[0]);

                // read recieved root
                int root;
                read(fd[0], &root, sizeof(root));

                cout << "Root of " << pid << " is " << root << endl;
                continue;
            }
            else if (p == 0)
            {
                close(fd[0]);
                dup(fd[1]);

                char *process = (char *)malloc(32 * sizeof(char));
                // get root of bug
                pair<int, int> p;
                p = getRoot(pid, suggest);
                if (suggest)
                    // kill all processes in the group
                    kill(-1 * p.second, SIGSEGV);
                // send back to parent process
                write(fd[1], &p.second, sizeof(p.second));
                exit(0);
            }
            else
            {
                perror("fork error");
                continue;
            }
        }

        // for any other command, it has to be run in a child process
        else {
            handle_process(piped_cmds_seq, &num_pipe_cmds, background_flag);
        }
 
    }

    return 0;
}
