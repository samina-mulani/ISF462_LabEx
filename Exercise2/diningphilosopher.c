#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#define EAT_TIME 1
#define THINK_TIME 1

int semid=-1;

void intHandler() //SIGINT handler to close semaphore set before exiting
{
	if(semctl(semid,0,IPC_RMID)==-1)
	{
		perror("rmid");
		exit(1);
	}
}

int main(int argc,char *argv[])
{	
	if(argc!=2)
	{
		printf("Incorrect number of arguments!\n");
		exit(1);
	}

	int n = atoi(argv[1]);

	if(n<=1) //assuming number of phil > 1
	{
		printf("Invalid number of philosophers (must be >1)!\n");
		exit(1);
	}

	int pid[n]; //to store process IDs of n philosopher processes
	for(int i=1;i<n;i++) pid[i]=-1;
	pid[0] = getpid();

	//create n semaphores (one for each fork)
	if((semid=semget(ftok("./diningphilosopher.c",0),n,IPC_CREAT|0666))==-1) 
	{
		perror("semget");
		exit(1);
	}

	//initialising semaphores to 1
	for(int i=0;i<n;i++)
	{
		if(semctl(semid, i, SETVAL, 1)==-1)
		{
			perror("setval");
			exit(1);
		}	
	}	

	struct sembuf sops[2];
	
	for(int i=0;i<n-1;i++) //creating n-1 processes
	{
		if(fork()) break;
		else
		{
			pid[i+1] = getpid();
		}
	}

	for(int i=0;i<n;i++)
	{
		if(getpid()==pid[i])
		{
			if(!i) //attach signal handler to process that created the sempahore set
				signal(SIGINT,intHandler);
			while(1)
			{
					sleep(THINK_TIME); //thinking
					 //philosopher i+1
					sops[0].sem_num = i%n;
					sops[0].sem_op = -1;
					sops[0].sem_flg = 0;
					sops[1].sem_num = (i+1)%n;
					sops[1].sem_op = -1;
					sops[1].sem_flg = 0;
					if(semop(semid,sops,2)==-1) //pickup forks
					{
						if(errno==EIDRM||errno==EINVAL||errno==EINTR) exit(0); //semaphore set has been removed 
						perror("semop1");
						exit(1);
					}

					//eat
					printf("Philospher %d with pid : %d eating!\n",i+1,getpid());
					sleep(EAT_TIME); //eating

					sops[0].sem_op = 1;
					sops[0].sem_flg = 0;
					sops[1].sem_op = 1;
					sops[1].sem_flg = 0;
					if(semop(semid,sops,2)==-1) //put down forks
					{
						if(errno==EIDRM||errno==EINVAL||errno==EINTR) exit(0); //semaphore set has been removed 
						perror("semop2");
						exit(1);
					}
			}
		}
		
	}
}