#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<sys/wait.h>
#include<time.h> 
#include <string.h>
#include<errno.h>

int gm=-1; //global value of second argument, M
int gsigc=0; //global value that holds count of SIGUSR1 signals received by a process

void evenhandler(int signo, siginfo_t *info, void *extra) //handler for even pid process
{
	gsigc++;
	printf("Process PID : %d\tSIGUSR1 received from PID : %d\tNumber of signals : %d\n",getpid(),info->si_pid,gsigc);
	if(gsigc>gm)
	{
		kill(info->si_pid,SIGTERM);
		sleep(1); //to make sure output of oddhandler gets printed
		kill(info->si_pid,SIGKILL);
		printf("Terminated Self\n");
		sleep(1); //to make sure above output gets printed
		kill(getpid(),SIGTERM);
	}
}

void oddhandler(int signo, siginfo_t *info, void *extra) //handler for odd pid process
{
	printf("Process PID : %d\tSIGTERM received from PID : %d\nTerminated by %d\n",getpid(),info->si_pid,info->si_pid);
}

int main(int argc,char* argv[])
{

	if(argc!=3)
	{
		if(argc<3) printf("You entered %d less arguments!\n",3-argc);
		if(argc>3) printf("You entered %d more arguments!\n",argc-3);
		printf(" **** Exiting **** \n");
		return 0;
	}
	
	//Getting parameters n and m
	int n = atoi(argv[1]);
	int m = atoi(argv[2]);
	gm=m; //so that the value of m is available to all processes

	//structure to hold even pids of processes
	int *epids = (int*)malloc(n*sizeof(int));
	for(int i=0;i<n;i++) epids[i] = -1;

	//Creating n processes
	for(int i=1,j=0;i<n;i++) //n-1 children created
	{
		if(getpid()%2==0) epids[j++] = getpid();

		if(fork()) break; //each child creates a child till n processes exist
	}

	gsigc=0; //signal count for even process at this point is 0

	//Even pid
	if(getpid()%2==0)
	{
		struct sigaction act1;
 		memset (&act1, '\0', sizeof(act1));
		act1.sa_sigaction = &evenhandler;
		act1.sa_flags = SA_SIGINFO;
 
		sigaction(SIGUSR1, &act1, NULL); 

		while(1) pause(); //waits for signal to be received
	}
	//Odd pid
	else 
	{
		struct sigaction act2;
 		memset (&act2, '\0', sizeof(act2));
		act2.sa_sigaction = &oddhandler;
		act2.sa_flags = SA_SIGINFO;
 
		sigaction(SIGTERM,&act2,NULL);

		//Getting processes created before it with even pid
		int size=0;
		while(epids[size]!=-1) size++;
		if(size==0) exit(0);		
	
		//send SIGUSR1 to randomly chosen index continuously
		while(1)
		{
			//choose a random index varying from 0 to size-1
			srand(time(0));
			int ind = rand()%size;

			if(kill(epids[ind],SIGUSR1)==-1) 
				//if(errno==ESRCH) perror("Error "); //if no process found, print error - prints infinitely if no even by pid process is left 
			
			sleep(1);		
		}
	}
	return 0;
}

