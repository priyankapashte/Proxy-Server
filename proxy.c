#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stddef.h>
#include <openssl/md5.h>
#include <time.h>

#define MAXBUFFSIZE 2048
#define MAXCLIENTS 100

char error404[] = "<html><title>HTTP/1.0 404</title><body> HTTP/1.0 404 Not Found: </body></html>";
char error4001[] = "<html><title>HTTP/1.0 400</title><body>HTTP/1.0 400 Bad Request: Invalid Method</body></html>";
char error4002[] = "<html><title>HTTP/1.0 400</title><body>HTTP/1.0 400Bad Request: Invalid URL</body></html>";
char error4003[] = "<html><title>HTTP/1.0 400</title><body> Bad Request: Invalid HTTP-Version:</body></html> ";
char error501[] = "<html><title>HTTP/1.0 501</title><body> HTTP/1.0 501 Not implemented: </body></html>";
char error500[] = "<html><title>HTTP/1.0 500 </title><body> HTTP/1.0 500 Internal Server Error: cannot allocate memory </body></html>\r\n\n ";


void error(char *msg);
void ServerConfig(char *port,int timer);
void Response(int n,int timer);
int HostConfig(const char *host,int portnum);

int clients_connected[MAXCLIENTS];

int main(int arg, char** argv)
{
	char *timeout;
	int timer = 0;
	timeout = argv[2];
	timer = atoi(timeout);
	ServerConfig(argv[1],timer);
	printf("Done\n");
	return 0;
}

void error(char *msg)    /* error - wrapper for perror */
{
	perror(msg);
  	exit(1);
}

void ServerConfig(char *port,int timer)
{
	int sock,option=1;
	struct sockaddr_in server, client;
	int sockaddr_len = sizeof(struct sockaddr_in);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("SOCKET FALIED");

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	server.sin_family = AF_INET;			// Address Family
	server.sin_port = htons(atoi(port));			// Sets port to network byte order
	server.sin_addr.s_addr = INADDR_ANY;	// Sets remote IP address
	bzero(&server.sin_zero,0);				// Zero the struct

	for(int i=0;i<MAXCLIENTS;i++)
	{
		clients_connected[i] = -1;
	}

	if((bind(sock, (struct sockaddr *)&server, sockaddr_len) == -1))
		error("BIND FAILED");

	if((listen(sock, MAXCLIENTS)) == -1)
		error("LISTEN FAILED");

	int client_num = 0;

	while(1)
	{
		//printf("In while\n");
		if((clients_connected[client_num] = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == -1)
			error("ACCEPT FAILED");
		printf("New Client connected from Port No. %d and IP %s\n",ntohs(client.sin_port), inet_ntoa(client.sin_addr));
		if(fork() == 0)
		{
			Response(client_num,timer);
		}		
		while(clients_connected[client_num]!=-1)
			client_num = (client_num+1)%MAXCLIENTS;
	}
}

void Response(int n,int timer)	// Send response to the Client Request
{
	char buffer[MAXBUFFSIZE],response[MAXBUFFSIZE], actualrequest[1000],cache[200],*cr=NULL,*cs=NULL,*min[3]={NULL},*cmin[3]={NULL},last_access[MAXBUFFSIZE],current_access[MAXBUFFSIZE];
	int nbytes,hsock;
	char *method,*url,*version,*host,*parturl,fullurl[1000],currenttime[30];
	int port = 80,min_diff,sec_diff,timeout=0,bytes_read;
	unsigned char abc[16];
	struct stat file_status;
	time_t mytime=time(NULL);
	long fsize;

	printf("In response..\n");
	bzero(buffer,MAXBUFFSIZE);
	nbytes = recv(clients_connected[n], buffer, MAXBUFFSIZE, 0);
	printf("Received\n");
	if(nbytes)
	{
		//printf("%s", buffer);
		method = strtok(buffer," ");
		printf("Method: %s\n",method);
		if(!strcmp(method,"GET"))
		{
			url = strtok(NULL," ");
			printf("URL: %s\n",url);
			if(strstr(url,"[") || strstr(url,"]"))
			{
				send(clients_connected[n],error4002,strlen(error4002),0);
    			printf("Invalid request\n");
			}
			else
			{
				version = strtok(NULL,"\n");
				printf("Version: %s\n",version);
				if(!strncmp(version,"HTTP/1.0",8))
				{
					host = strtok(url,"/");
					host = strtok(NULL,"/");
					printf("Host: %s\n",host);	
					parturl = strtok(NULL," ");
					hsock = HostConfig(host,port);

					bzero(fullurl,1000);
					if(parturl == NULL)
					{
						sprintf(actualrequest,"GET / HTTP/1.0\r\nHost: %s\r\nConnection: closed\r\n\r\n",host);
						strcpy(fullurl,host);
					}
					else
					{
						sprintf(actualrequest,"GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: closed\r\n\r\n",parturl,host);
						strcpy(fullurl,host);
						strcat(fullurl,"/");
    			     	strcat(fullurl,parturl);
					}
					
					
    				printf("Actual request is %s\n",actualrequest);
    				
    				printf("Fullurl: %s\n",fullurl);

    				//Calculate MD5SUM

    				MD5_CTX mdContext;
			  		MD5_Init (&mdContext);
			  		MD5_Update (&mdContext,fullurl,1000);
			  		MD5_Final (abc,&mdContext);
			  		char *out = (char*)malloc(33);

			   		for (int np = 0; np < 16; ++np) 
			     	snprintf(&(out[np*2]), 16*2, "%02x", (unsigned int)abc[np]);
			        

			        bzero(cache,200);
			        strcpy(cache,out);
			        strcat(cache,".html");

			        printf("MD5SUM %s\n",out);
			        printf("Cache: %s\n",cache);

			        FILE *fp = fopen(cache,"r");
			        if(fp!=NULL)
			        {
			        	printf("Cache is present..\n");
			        	stat(cache,&file_status);
			        	printf("Time of last access: %ld : %s",file_status.st_atime, ctime(&file_status.st_atime));
			        	strcpy(last_access,ctime(&file_status.st_atime));
			        	printf("Last Access: %s\n",last_access);
			        	cr=strdup(last_access);
			            min[0]= strtok(cr,":");
			            min[1]=strtok(NULL,":"); //minutes
			            min[2]=strtok(NULL,"  \n");
			            printf("Last Access Minutes %s\n",min[1]);
			            printf("Last Access Seconds %s\n\n",min[2]);

			            strcpy(currenttime,ctime(&mytime));
				        printf("Current Time %s\n",currenttime);
				        cs=strdup(currenttime);
			          	cmin[0]= strtok(cs,":");
			          	cmin[1]=strtok(NULL,":"); //minutes
			          	cmin[2]=strtok(NULL,"  \n"); //sec
			            printf("Current Minutes %s\n",cmin[1]);
			          	printf("Current Seconds %s\n",cmin[2]);

			          	sec_diff = 0;

			          	sec_diff = ((atoi(cmin[1])*60)+cmin[2]) - ((atoi(min[1])*60)+min[2]);

			        //  	min_diff=atoi(cmin[1])-atoi(min[1]);
			        //   	sec_diff=atoi(cmin[2])-atoi(min[2]);
			        //   	printf("Minute difference is %d\n",min_diff);
					//   	printf("Seconds difference is %d\n",sec_diff);

					//    sec_diff = min_diff*60+sec_diff;

					    printf("Difference in Seconds: %d\n",sec_diff);

				    	printf("Seconds: %d\n",sec_diff);
           				printf("Timer: %d\n",timer);

           				if(sec_diff>timer)
			            {
			            	printf("Timeout has occured...\n");
			          		printf("Not going to cache\n");
			          	 	sec_diff=0;
			          	 	timeout=1;
			          	 	fp= fopen(cache,"w+");
	    					if(fp==NULL)
							{
				  				printf("Can't create file\n");
				  			}

				  			nbytes = send(hsock,actualrequest,strlen(actualrequest),0);

					    	while((nbytes = recv(hsock,response,MAXBUFFSIZE,0))>0)
					    	{
					    		//printf("Response: %s\n",response);
					    		if(!(nbytes<=0))
									send(clients_connected[n],response,nbytes,0);
								fprintf(fp,"%s",response);
								bzero(response,MAXBUFFSIZE);
					    	}
					    	fclose(fp);
				        }
			          	else
			          	{
			          		printf("Fetching file from Cache.....\n");
			          		fseek(fp,0,SEEK_SET);
			          		if ((fsize=fseek(fp, 0, SEEK_END) == -1))
					           	printf("The file was not seeked.\n");
					        fsize = ftell(fp);
					        if (fsize == -1)
					           	printf("The file size was not retrieved");
					        fseek(fp,0,SEEK_SET);
					        printf("File size: %ld\n", fsize);
					        char cachebuff[fsize];
					        bytes_read=fread(cachebuff,1,fsize,fp);
					        if((nbytes = send(clients_connected[n],cachebuff,bytes_read,0))<0)
								printf("Error sending file \n");
							fclose(fp);
			          	}


			        }
       				else
       				{
       					printf("Cache is not present\n");
       					printf("Creating cache...\n");
       					fp= fopen(cache,"w+");
    					if(fp==NULL)
						{
			  				printf("Can't create file\n");
			  			}

			  			nbytes = send(hsock,actualrequest,strlen(actualrequest),0);

				    	while((nbytes = recv(hsock,response,MAXBUFFSIZE,0))>0)
				    	{
				    		//printf("Response: %s\n",response);
				    		if(!(nbytes<=0))
								send(clients_connected[n],response,nbytes,0);
							fprintf(fp,"%s",response);
							bzero(response,MAXBUFFSIZE);
				    	}
				    	fclose(fp);
       				}

				}
				else
				{
					send(clients_connected[n],error4003,strlen(error4003),0);
    				printf("Invalid version\n");
				}
			}
				
			
		}
		else 
		{
			send(clients_connected[n],error4001,strlen(error4001),0);
    		printf("Invalid request Method\n");
		}

	}
	else
	{
		send(clients_connected[n],error500,strlen(error500),0);
    	printf("Invalid request\n");
	}
	printf("Closing socket....\n");
	shutdown(clients_connected[n],SHUT_RDWR);
	close(clients_connected[n]);	
	clients_connected[n]=-1;
}

int HostConfig(const char *host,int portnum)
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      portnum   - server port number
 */
{
        struct hostent  *phe;   /* pointer to host information entry    */
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s;              /* socket descriptor                    */


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

    /* Map port number (char string) to port number (int)*/
        if ((sin.sin_port=htons(portnum)) == 0)
                error("Can't get port number\n");

    /* Map host name to IP address, allowing for dotted decimal */
        if ((phe = gethostbyname(host)))
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
      //  else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
        //        errexit("can't get \"%s\" host entry\n", host);

    /* Allocate a socket */
        s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0)
                error("Can't create socket");

    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
                printf("Can't connect");
        return s;
}