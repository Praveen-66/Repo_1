#ifndef MSH_H
#define MSH_H

//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>


//Process Structure
typedef struct process
{
    struct process *next;
    pid_t pid;
    char **argv;
    char completed;
    char stopped;
    int status;
}process;

//Job structure
typedef struct job
{
    struct job *next;
    int j_no;
    char *command;
    process *first_process;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int stdin, stdout, stderr;
}job;

//Stopped job structure
typedef struct sjob
{
    pid_t pgid;
    struct sjob *next;
}Slist;

//Global variables
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal, shell_is_interactive;
job *first_job;
Slist *sjob;
char PS1[300], path[200];
char ex_path[300];
char prompt_ch;
int status, exit_st;


/* Function declaration */

//Initialising shell
void init_shell(void);

//To put jobs in foreground and background
void put_job_fg(job *j, int cont);
void put_job_bg(job *j, int cont);

//To parseline and identify the commands
job* parseline(char **, char*, int*);

//List functions
int insert_job(job **head, char*);
int insert_process(process**, char**);
int insert_sjob(Slist **head, pid_t);
int delete_sjob(Slist **);

//To launch jobs and processes
void launch_job (job *j, int foreground);
void launch_process (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground);

//To execute builtin commands
int builtin_cmd(char *cmd[]);

//To continue the stopped jobs
void continue_job (job *j, int foreground);

//Job utility functions
job * find_job (pid_t pgid);
int job_is_stopped(job *j);
int job_is_completed(job *j);
void delete_completed_job();
void free_job(job *j);

#endif
