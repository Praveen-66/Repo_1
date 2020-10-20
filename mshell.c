#include "mshell.h"
#include "list_func.c"
#include "update_status.c"

int main(int argc, char*argv[])
{
    //Declaring the local variables
    char **cmd = malloc(sizeof(char*)), *cmdline = malloc(80);
    int i = 0;

    //Initialising the global variables
    prompt_ch = 1;
    first_job = NULL;
    sjob = NULL;

    //Get current working directory
    getcwd(path, 200);

    //Stores the current executable path
    sprintf(ex_path, "%s%s", path, argv[0]+1);

    //Initialising the shell
    init_shell();

    //Clear screen
    system("clear");
    
    while (1)
    {
	//Declaring the local variables	
	job *j;
    	int foreground = 1;
	
	//Get current working directory
    	getcwd(path, 200);

	//Formating the prompt message
	if (prompt_ch)
	{
	    sprintf(PS1, "\033[1;32mmsh\033[0m:\033[1;34m%s\033[0m$ ", path);
	}

	//Reading the commands
	fflush(stdin);
	cmdline = readline(PS1);
	fflush(stdout);

	//Checks for empty commands
	if (!strlen(cmdline))
	    continue;

	//Parsing the commands and executing it
	if (j = parseline(cmd, cmdline, &foreground))
	{
	    //Executes the external commands
	    launch_job(j,foreground);
	}

	//Deletes the terminated jobs from active job list
	delete_completed_job();
    }
}

/* Signal Handler */
void sigmain_handler (int sig_num, siginfo_t *info, void *nothing)
{
    job *j;

    //For SIGCHLD signal
    if (sig_num == SIGCHLD)
    {
//	printf("Process %d : %d\n", info -> si_pid, info -> si_status);    

	//Terminated child process
    	if (!info -> si_status 
    		|| info -> si_code == CLD_EXITED
    		|| info -> si_code == CLD_DUMPED
    		|| info -> si_code == CLD_KILLED)
	{
	    //Finds the job in active job list
	    if (!(j = find_job(getpgid(info -> si_pid))))
	    	j = find_job(info -> si_pid);

	    //Marks as completed job
	    if (j)
		j -> first_process -> completed = 1;

	    //Exit status
	    if (info -> si_status)
	    	exit_st = 128 + info -> si_status;
	    else
	    	exit_st = info -> si_status;
	}

	//Stopped child process
	if (info -> si_code == CLD_STOPPED)
	{
	    //Finds the job in active job list
	    j = find_job(getpgid(info -> si_pid));

	    //Marks as stopped job
	    if (j)
	    {
		j -> first_process -> stopped = 1;
		insert_sjob(&sjob, info -> si_pid);
	    }
	    printf("\n[%d]+  %s\t\t\t%s\n", j -> j_no, "Stopped", j -> command);

	    //Exit status
	    exit_st = 128 + info -> si_status;
	}
	
	//Continued child process
	if (info -> si_code == CLD_CONTINUED)
	{
	    //Finds the job in active job list
	    j = find_job(getpgid(info -> si_pid));

	    //Unmarks stopped job
	    if (j)
		j -> first_process -> stopped = 0;
	}
    }

    //For SIGINT signal
    if (sig_num == SIGINT)
    {
    	getcwd(path, 200);
	sprintf(PS1, "\n\033[1;32mmsh\033[0m:\033[1;34m%s\033[0m$ ", path);
	printf("%s", PS1);
	    exit_st = 128 + SIGINT;
    }
}

/* Initialising the shell */
void init_shell()
{
    //Initialising the global variables
    shell_terminal = 0;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive)
    {
    	//Kills all the terminal foreground processes
    	while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
	{
	    kill(-shell_pgid, SIGTTIN);
	}

	//Declaring the action for handler
	struct sigaction act1;
	act1.sa_sigaction = sigmain_handler;
	act1.sa_flags = SA_SIGINFO;

	//Registering the signals
	sigaction(SIGCHLD, &act1, NULL);
	sigaction(SIGINT, &act1, NULL);

	//Ignoring the signals
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);
	signal (SIGTTIN, SIG_IGN);
	signal (SIGTTOU, SIG_IGN);
	
	//Storing the shell pid
	shell_pgid = getpid();
	
	//Sets the process group
	if (setpgid(shell_pgid, shell_pgid) < 0)
	{
	    perror("setpgid");
	    exit(1);
	}

	//Sets the terminal foreground as shell
	tcsetpgrp(shell_terminal, shell_pgid);

	//Gets the terminal attributes
	tcgetattr(shell_terminal, &shell_tmodes);
    }
}

/* Parsing the commands */
job* parseline(char *cmd[], char *cmdline, int *foreground)
{
    //Declaring and initialising the local variables
    int p_ct = 0, *ind = malloc(sizeof(int*)), i = 0;
    char* str, cmdl[strlen(cmdline)];
    *foreground = 1;

    //Stores the first cmd index
    ind[p_ct] = 0;
    job *jb = NULL; 

    //Copies the command line string
    strcpy(cmdl, cmdline);

    //Extracing each commands
    str = strtok(cmdline, " ");
    while (str)
    {
	cmd[i] = malloc(strlen(str));

	//Checks for pipe
	if (!strcmp(str, "|"))
	{
	    //Replaces pipe with NULL
	    cmd[i++] = NULL;
	    p_ct++;

	    //Stores the next command index
	    ind[p_ct] = i;
	}

	//Checks for &
	else if (!strcmp(str, "&"))
	{
	    //Replaces & with NULL
	    cmd[i++] = NULL;

	    //Sets to background
	    *foreground = 0;
	}

	//Stores the command
	else
	{
	    strcpy(cmd[i++], str);
	}
	str = strtok(NULL, " ");
    }
    cmd[i] = NULL;

    //To change the prompt
    if (!strncmp(cmd[0], "PS1=", 4) && strncmp(cmd[0], "PS1= ", 5))
    {
	strcpy(PS1, cmd[0]+4);
	prompt_ch = 0;
    }

    //bg command
    else if (!strcmp(cmd[0], "bg"))
    {
    	//Declaring a local variable
	job *j;

	//Checks for available jobs
	if (sjob || (j = first_job))
	{
	    //If it is stopped job
	    if (sjob)
	    {
		//Finds the job in active job list
		j = find_job(sjob -> pgid);

		//Deletes the job from stopped job list
		delete_sjob(&sjob);
	    }

	    //If job is completed
	    if (j -> first_process -> completed)
	    {
		//Prints the message
		fprintf(stderr, "msh: %s: job has terminated\n", cmd[0]);
		printf("[%d]  %s\t\t\t%s\n", j -> j_no, "Done", j -> command);
	    }
	    //If job is running
	    else if (!j -> first_process -> stopped)
		//Prints the message
		fprintf(stderr, "msh: %s: job %d already in background\n", cmd[0], j -> j_no);

	    //Continues the job in the background
	    else
		continue_job(j, 0);
	}

	//If no jobs available
	else
	    fprintf(stderr, "msh: %s: current: no such job\n", cmd[0]);

	exit_st = 0;
    }

    //fg command
    else if (!strcmp(cmd[0], "fg"))
    {
    	//Declaring a local variable
	job *j;

	//Checks for available jobs
	if (sjob || (j = first_job))
	{
	    //If it is stopped job
	    if (sjob)
	    {
		//Finds the job in active job list
		j = find_job(sjob -> pgid);

		//Deletes the job from stopped job list
		delete_sjob(&sjob);
	    }
	    //If job is completed
	    if (j -> first_process -> completed)
	    {
		//Prints the message
		fprintf(stderr, "msh: %s: job has terminated\n", cmd[0]);
		printf("[%d]  %s\t\t\t%s\n", j -> j_no, "Done", j -> command);
	    }
	    //Continues the job in the foreground
	    else
		continue_job(j, 1);
	}

	//If no jobs available
	else
	    fprintf(stderr, "msh: %s: current: no such job\n", cmd[0]);

	exit_st = 0;
    }

    //jobs command
    else if (!strcmp(cmd[0], "jobs"))
    {
    	//Declaring and initialising the local variables
	job* j, *jnext, *jlast = NULL;

	//Checks the jobs list
	for (j = first_job; j; j = jnext)
	{
	    jnext = j->next;

	    //If completed notifies and deletes the jod
	    if (job_is_completed (j)) 
	    {
		//Prints the message
		printf("[%d]  %s\t\t\t%s\n", j -> j_no, "Done", j -> command);
		j->notified = 1;

		if (jlast)
		    jlast->next = jnext;
		else
		    first_job = jnext;

		//Frees the job
		free_job (j);
	    }

	    //If job is stopped
	    else if (job_is_stopped (j)) 
	    {
		//Prints the message
		printf("[%d]  %s\t\t\t%s\n", j -> j_no, "Stopped", j -> command);
		jlast = j;
	    }

	    //If job is running
	    else
	    {
		//Prints the message
		printf("[%d]  %s\t\t\t%s\n", j -> j_no, "Running", j -> command);
		jlast = j;
	    }
	}
	exit_st = 0;
    }

    //Checks for builtin commands
    else if (!builtin_cmd(cmd))
    {
    	//Insert a new job
	insert_job(&first_job, cmdl);
	for (jb = first_job; jb -> next; jb = jb -> next);

	for (int j = 0 ; j <= p_ct; j++)
	{
	    //Declaring and initialising the local variables
	    char **argv = malloc(sizeof(char*));
	    int i = ind[j], k = 0;

	    //Copies the commands
	    while (cmd[i])
	    {
		argv[k] = malloc(strlen(cmd[i]));
		strcpy(argv[k++], cmd[i++]);
	    }
	    argv[k] = NULL;

	    //Inserts the new process
	    insert_process( &(jb -> first_process), argv);
	}
	free(ind);
    }

    //Clears all the commands
    for(int j = 0; j < i; j++)
    {
	cmd[j] = NULL;
	free(cmd[j]);
    }
    free(cmdline);

    return jb;
}

/* To execute the builtin commands */
int builtin_cmd(char *cmd[])
{
    //Declaring the local variables
    char *builtin_cmds[4] = {"exit", "pwd", "cd", "echo"}, path[200];
    int i;

    //Checks for the builtin commands
    for (i = 0; i < 4; i++)
    {
	if (!strcmp(cmd[0], builtin_cmds[i]))
	{
	    break;
	}
    }

    switch (i)
    {
	//exit command
	case 0:
	    exit_st = 0;
	    exit(0);
	    break;

	    //pwd command
	case 1:
	    //Get the current working directory
	    if (getcwd(path, 200))
	    {
		printf("%s\n", getcwd(path, 200));
	    }
	    exit_st = 0;
	    break;

	    //cd command
	case 2:
	    //Changes the directory
	    if (chdir(cmd[1]))
	    {
		printf("msh: %s: %s: No such file or directory\n", cmd[0], cmd[1]);
	    }
	    exit_st = 0;
	    break;

	    //echo command
	case 3:
	    //To print the shell pid
	    if (!strcmp(cmd[1], "$$"))
	    {
	    	printf("%d\n", getpid());
	    }

	    //To print the previous process's exit status
	    else if (!strcmp(cmd[1], "$?"))
	    {
	    	printf("%d\n", exit_st);
	    }

	    //Prints the executable path
	    else if (!strcmp(cmd[1], "$SHELL"))
	    {
		printf("%s\n", ex_path);
	    }

	    //Prints the message
	    else
	    {
		int j = 1;
		while (cmd[j])
		{
		    printf("%s ", cmd[j++]);
		}
		printf("\n");
	    }
	    exit_st = 0;
	    break;

	    //If not a builtin command
	default:
	    return 0;
    }
    return 1;
}

/* To launch a job */
void launch_job (job *j, int foreground)
{
    //Declaring the local variables
    int mypipe[2], infile, outfile;
    pid_t pid;
    process *p;

    //Initialising the infile fd to 0
    infile = 0;

    for (p = j->first_process; p; p = p->next)
    {
	// Set up pipes, if necessary.
	if (p->next)
	{
	    if (pipe (mypipe) < 0)
	    {
		perror ("pipe");
		exit (1);
	    }
	    outfile = mypipe[1];
	}
	else
	    outfile = 1;

	// Fork the child processes.
	pid = fork ();

	if (pid == 0)
	{
	    // Child process
	    launch_process (p, j->pgid, infile, outfile, 2, foreground);
	}
	else if (pid < 0)
	{
	    //fork failed
	    perror ("fork");
	    exit (1);
	}
	else
	{
	    //Parent process.
	    p->pid = pid;
	    if (shell_is_interactive)
	    {
		if (!j->pgid)
		    j->pgid = pid;

		//Sets the process group
		setpgid (pid, j->pgid);
	    }
	}

	// Clean up after pipes
	if (infile != 0)
	    close (infile);
	if (outfile != 1)
	    close (outfile);

	infile = mypipe[0];
    }

    //If foreground process
    if (foreground)
    {
	put_job_fg(j, 0);
    }
    //If background process
    else
    {
	printf("[%d] %d\n", j -> j_no, j -> pgid);
	put_job_bg(j, 0);
    } 
}

/* To launch a process */
void launch_process (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground)
{
    //Declaring a local variables
    pid_t pid;

    if (shell_is_interactive)
    {
	pid = getpid ();
	if (pgid == 0)
	{
	    pgid = pid;
	}

	//Sets the process group
	setpgid (pid, pgid);

	//Sets the terminal foreground process
	if (foreground)
	{
	    tcsetpgrp (shell_terminal, pgid);
	}

	//Setting the signals to default
	signal (SIGINT, SIG_DFL);
	signal (SIGQUIT, SIG_DFL);
	signal (SIGTSTP, SIG_DFL);
	signal (SIGTTIN, SIG_DFL);
	signal (SIGTTOU, SIG_DFL);
	signal (SIGCHLD, SIG_DFL);
    }

    // Set the standard input/output channels of the new process.
    if (infile != 0)
    {
	dup2 (infile, 0);
	close (infile);
    }
    if (outfile != 1)
    {
	dup2 (outfile, 1);
	close (outfile);
    }
    if (errfile != 2)
    {
	dup2 (errfile, 2);
	close (errfile);
    }

    // Exec the new process
    execvp (p->argv[0], p->argv);
    perror ("execvp");
    exit (1);
}
