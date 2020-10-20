#include "mshell.h"

/* To insert a process into the job list */
int insert_process(process **head, char **data)
{
    //1. Create a node
    process *new = malloc(sizeof(process));

    //2. Check whether node is created or not
    if (!new)
    {
    	return 1;
    }

    //3. Update the data and the link
    new -> argv = data;
    new -> next = NULL;
    new -> completed = 0;
    new -> stopped = 0;

    //4. Check whether the list is empty
    if (!*head)
    {
    	//4.1 Update the head
    	*head = new;
	return 0;
    }
    //5. Take a local reference of head
    process *temp;
    temp = *head;

    //5.1 Traverse till the last node
    while (temp -> next)
    {
	temp = temp -> next;
    }

    //5.2 Establish link betweeen last + new node
    temp -> next = new;

    return 0;
}

/* To insert a job into the job list */
int insert_job(job **head, char *cmd)
{
    //1. Create a node
    job *new = malloc(sizeof(job));

    //2. Check whether node is created or not
    if (!new)
    {
    	return 1;
    }

    //3. Update the data and the link
    new -> next = NULL;
    new -> first_process = NULL;
    new -> pgid = 0;
    new -> command = strdup(cmd);
    new -> notified = 0;

    //4. Check whether the list is empty
    if (!*head)
    {
    	//4.1 Update the head
    	new -> j_no = 1;
    	*head = new;
    	return 0;
    }

    //5. Take a local reference of head
    job *temp;
    temp = *head;
    int i = 2;

    //5.1 Traverse till the last node
    while (temp -> next)
    {
	i++;
	temp = temp -> next;
    }

    //5.2 Establish link betweeen last + new node
    new -> j_no = i;
    temp -> next = new;

    return 0;
}

/* To insert a stopped job into the sjob list */
int insert_sjob(Slist **head, pid_t data)
{
    //1. Create a node
    Slist *new = malloc(sizeof(Slist));

    //2. Check whether node is created or not
    if (!new)
    {
    	return 1;
    }

    //3. Update the data and the link
    new -> pgid = data;
    new -> next = NULL;

    //4. Check whether the list is empty
    if (!*head)
    {
    	//4.1 Update the head
    	*head = new;
    	return 0;
    }
    //5.1 Establish link between new + first node
    new -> next = *head;

    //5.2 Update the head
    *head = new;

    return 0;
}

/* T0 delete the finished stopped job from sjob list */
int delete_sjob(Slist **head)
{
    //1. Check whether the list is empty
    if (!*head)
    {
	return 1;
    }

    //Take a local reference of head
    Slist *temp = *head;

    //2.1 Establishes link to next node
    *head = (*head) -> next;

    //2.2 Frees the node memory
    free(temp);

    return 0;
}
