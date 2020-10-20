#include "mshell.h"

/* Delete terminated jobs from the active job list */
void delete_completed_job()
{
    //Declaring the local variables
    job *j, *jlast, *jnext;
    jlast = NULL;

    //Checks all available jobs
    for (j = first_job; j; j = jnext)
    {
	jnext = j->next;

	//If completed deletes it from job list
	if (job_is_completed(j)) 
	{
	    if (jlast)
		jlast->next = jnext;
	    else
		first_job = jnext;

	    //Frees the job
	    free_job (j);
	}
	else
        jlast = j;
    }
}


/* Continue the job */
void continue_job (job *j, int foreground)
{
    if (foreground)
    {
    	//Prints the message
	if (j -> command[strlen(j -> command) - 1] == '&')
	{
	    j -> command[strlen(j -> command) - 1] = '\0';
	}
	printf("%s\n", j -> command);

	//Runs the job in foreground
	put_job_fg(j, 1);
    }

    else
    {
    	//Prints the message
	if (j -> command[strlen(j -> command) - 1] != '&')
	{
	    j -> command[strlen(j -> command)] = ' ';
	    j -> command[strlen(j -> command)] = '&';
	    j -> command[strlen(j -> command)] = '\0';
	}
	printf("[%d]+ %s\n", j -> j_no, j -> command);

	//Runs the job in background
	put_job_bg(j, 1);
    }
}

/* Put the job in foreground */
void put_job_fg(job *j, int cont)
{
    //Declaring the local variable
    process  *p;

    //Sets terminal foreground process group
    tcsetpgrp(shell_terminal, j-> pgid);

    //To continue the stopped job
    if (cont)
    {
	//To set the terminal attributes to the job
    	tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);

    	//Sends continue signal
	kill(-(j -> pgid), SIGCONT);

    	//Waits for all processes
	if (waitpid (-j -> pgid, &status, WUNTRACED) == -1)
	    waitpid (-j -> pgid, &status, WUNTRACED);
    }
    else
    {
    	//Waits for all processes
	for (p = j -> first_process; p; p = p -> next)
	    waitpid (p -> pid, &status, WUNTRACED);
    }
  
    //Sets terminal foreground process group
    tcsetpgrp(shell_terminal, shell_pgid);

    //To get the terminal attributes from the job
    tcgetattr(shell_terminal, &j -> tmodes);

    //To set the terminal attributes to the shell
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Put the job in background */
void put_job_bg(job *j, int cont)
{
    //To continue the stopped job
    if (cont)
    {
    	//Sends continue signal
    	kill(-(j -> pgid), SIGCONT);
    }
}

/* Checks for completed jobs */
int job_is_completed(job *j)
{
    process *p = j -> first_process;
    while (p)
    {
    	if (!p -> completed)
	{
    	    return 0;
	}
	else
    	    return 1;
	p = p -> next;
    }
    return 1;
}

/* Checks for stopped jobs */
int job_is_stopped(job *j)
{
    process *p = j -> first_process;
    while (p)
    {
    	if (!p -> stopped)
	{
    	    return 0;
	}
	else
    	    return 1;
	p = p -> next;
    }
    return 1;
}

/* Find the active job with the indicated pgid */
job * find_job (pid_t pgid)
{
  for (job *j = first_job; j; j = j->next)
  {
    if (j->pgid == pgid)
      return j;
  }
  return NULL;
}

/* To free the terminated job and its processes */
void free_job(job *j)
{
    process *p = j -> first_process, *pnext;
    while (p)
    {
    	pnext = p -> next;
    	free(p -> argv);
    	free(p);
    	p = pnext;
    }
    free(j);
}
