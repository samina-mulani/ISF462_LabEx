#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#define MAX 200
#define WAIT_TIME_SEC 60 //when checking connectivity to ports 1-1024, will wait twice the time specified
#define WAIT_TIME_MICROSEC 0

int notEmpty(int sockfds[],int len)
{
	for(int i=0;i<len;i++)
		if(sockfds[i]!=-1)
			return 1;
	return 0;
}

int main()
{
	//ASSUMPTION 1 - URLs start with protocol name followed by colon and optional forward slashes
	//ASSUMPTION 2 - Webserver port runs on TCP port 80, and if this exists, only then are ports 1-1024 checked (for the first IP address that returns success on existtence of TCP port 80)
	
	char *fname = "webpages.txt";
	char url[MAX],domain[MAX],path[MAX];
	FILE *fp = fopen(fname,"r");
	if(fp==NULL)
	{
		printf("No file webpages.txt found!\n");
		exit(1);
	}
	while(fgets(url,MAX,fp)) //read one URL at a time
	{
		url[strcspn(url,"\n")] = '\0';
		memset(domain,0,sizeof(domain));
		memset(path,0,sizeof(path));
		int r = sscanf(url,"%*[^:]%*[:/]%[^/]%s",domain,path); //parsing URL
		if(!(r==1||r==2)) //Allows if path is empty
		{
			printf("\"%s\" not in correct format!\n\n",url);
			continue;
		}
		if(r==2)
			printf("Domain : %s \tPath : %s\n",domain,path);
		else if(r==1)
			printf("Domain : %s \tPath : N/A\n",domain);

		struct addrinfo hints,*res;
		memset(&hints, 0, sizeof(struct addrinfo));	
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM; //TCP (see ASSUMPTION 2)

		int err;

		err=getaddrinfo(domain,"80",&hints,&res);
		if(err!=0)
		{
			printf("%s \n",gai_strerror(err));
			printf("Domain DOES NOT exist!\n\n");
			continue;
		}
		else printf("Domain exists!\n");;
	
		int flgFound=0;
		struct timeval timeout;

		printf("Checking if webserver on port 80 exists (Please wait approximately (Number of IP addresses)*%d seconds maximum - configurable) ...\n",WAIT_TIME_SEC);
		while(res!=NULL)
		{
				int sfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
				if(sfd<0)
				{
					printf("Error in assigning socket fd\n");
					exit(1);
				}
				
				timeout.tv_sec = WAIT_TIME_SEC;
				timeout.tv_usec = WAIT_TIME_MICROSEC;

				//block connect for a maximum period specified by timeout
				setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
				setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

				if(connect(sfd,res->ai_addr,res->ai_addrlen)==0) //found
				{
					flgFound=1;
					close(sfd);
					break;
				}
				
			res=res->ai_next;
			close(sfd);
		}

		if(flgFound)
		{
			printf("Domain runs a webserver at port 80!\n");
		}
		else
		{
			printf("Domain DOES NOT run a webserver at port 80!\n\n");
			continue; //(see ASSUMPTION 2)
		}

		int success[1024]; //stores which ports have successfully been connected with
		for(int i=0;i<1024;i++) success[i] = 0;

		printf("Checking for active services on ports 1-1024 (Please wait approximately a maximum of %d seconds - configurable ) ...\n",2*WAIT_TIME_SEC);

		//non blocking connect on first 512 ports
		fd_set rset,wset,oldrset,oldwset;
		int max,error;
		socklen_t len;
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&oldrset);
		FD_ZERO(&oldwset);
		int sockfds[512];

		for(int i=0;i<512;i++) 
		{
			sockfds[i] = -1;
			sockfds[i] = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
			if(sockfds[i]<0)
			{
				printf("Error in assigning socket fd\n");
				exit(1);
			}
			int flags = fcntl(sockfds[i],F_GETFL,0);
			fcntl(sockfds[i],F_SETFL,flags|O_NONBLOCK);

			if(res->ai_family==AF_INET) //IPv4
				((struct sockaddr_in*)(res->ai_addr))->sin_port = htons(i+1);
			else if(res->ai_family==AF_INET6) //IPv6
				((struct sockaddr_in6*)(res->ai_addr))->sin6_port = htons(i+1);

			if(connect(sockfds[i],res->ai_addr,res->ai_addrlen)<0)
			{
				FD_SET(sockfds[i],&rset);
				wset=rset;
				max=sockfds[i];
			}
			else
				{printf("Connected to port %d immediately ...\n",i+1);success[i] = 1;} //connect successful immediately
		}

		timeout.tv_sec  = WAIT_TIME_SEC;  
		timeout.tv_usec = WAIT_TIME_MICROSEC;
		int code;
		oldrset = rset;
		oldwset = wset;

		while(notEmpty(sockfds,512)&&(code=select(max+1,&rset,&wset,NULL,&timeout)))
		{
			if(code==-1)
			{
				perror("select");
				break;
			}

			for(int i=0;i<512;i++)
			{
				if(FD_ISSET(sockfds[i],&rset)||FD_ISSET(sockfds[i],&wset))
				{
					if(getsockopt(sockfds[i],SOL_SOCKET,SO_ERROR,&error,&len)==0)
					{
						printf("Connected to port %d ...\n",i+1);
						success[i] = 1;
						FD_CLR(sockfds[i],&oldrset);
						FD_CLR(sockfds[i],&oldwset);
						close(sockfds[i]);
						sockfds[i]=-1;
					}
					else
					{
						FD_CLR(sockfds[i],&oldrset);
						FD_CLR(sockfds[i],&oldwset);
						close(sockfds[i]);	
						sockfds[i]=-1;
					}
				}	
			}

			rset = oldrset;
			wset = oldwset;
		}

		//select timed out		
		
		for(int i=0;i<512;i++)
		{
			if(sockfds[i]!=-1)
				close(sockfds[i]);
		}

		//non blocking connect on next 512 ports
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&oldrset);
		FD_ZERO(&oldwset);

		for(int i=0;i<512;i++) 
		{
			sockfds[i] = -1;
			sockfds[i] = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
			if(sockfds[i]<0)
			{
				printf("Error in assigning socket fd\n");
				exit(1);
			}
			int flags = fcntl(sockfds[i],F_GETFL,0);
			fcntl(sockfds[i],F_SETFL,flags|O_NONBLOCK);

			if(res->ai_family==AF_INET) //IPv4
				((struct sockaddr_in*)(res->ai_addr))->sin_port = htons(i+513);
			else if(res->ai_family==AF_INET6) //IPv6
				((struct sockaddr_in6*)(res->ai_addr))->sin6_port = htons(i+513);

			if(connect(sockfds[i],res->ai_addr,res->ai_addrlen)<0)
			{
				FD_SET(sockfds[i],&rset);
				wset=rset;
				max=sockfds[i];
			}
			else
				{printf("Connected to port %d immediately ...\n",i+513);success[i+513] = 1;} //connect successful immediately
		}

		timeout.tv_sec  = WAIT_TIME_SEC;  
		timeout.tv_usec = WAIT_TIME_MICROSEC;
		int code2;
		oldrset = rset;
		oldwset = wset;

		while(notEmpty(sockfds,512)&&(code2=select(max+1,&rset,&wset,NULL,&timeout)))
		{
			if(code2==-1)
			{
				perror("select");
				break;
			}

			for(int i=0;i<512;i++)
			{
				if(FD_ISSET(sockfds[i],&rset)||FD_ISSET(sockfds[i],&wset))
				{
					if(getsockopt(sockfds[i],SOL_SOCKET,SO_ERROR,&error,&len)==0)
					{
						printf("Connected to port %d ...\n",i+513);
						success[i+513] = 1;
						FD_CLR(sockfds[i],&oldrset);
						FD_CLR(sockfds[i],&oldwset);
						close(sockfds[i]);
						sockfds[i]=-1;
					}
					else
					{
						FD_CLR(sockfds[i],&oldrset);
						FD_CLR(sockfds[i],&oldwset);
						close(sockfds[i]);	
						sockfds[i]=-1;
					}
				}	
			}

			rset = oldrset;
			wset = oldwset;
		}

		//select timed out	

		for(int i=0;i<512;i++)
		{
			if(sockfds[i]!=-1)
				close(sockfds[i]);
		}
		
		printf("Ports from 1-1024 in use : ");
		for(int i=0;i<1024;i++)
		{
			if(success[i])
			{
				printf("  %d  ",i+1);
			}
		}


		printf("\n");

		freeaddrinfo(res);
		printf("\n");
	}

	fclose(fp);

	return 0;
}