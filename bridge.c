


#include<pthread.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include <fcntl.h>

struct btable				// creating the structure for bridge table
{
	int sockfd;
	unsigned char macaddr[1024];
	char interface[512];
	int port;
	int timer;
}btable[20];

struct ippacket				// creating the structure for ippacket
{
        unsigned char sourceip[1024];
        unsigned char destinip[1024];
	unsigned char sourcemac[1024]; //parameter not used but defined inorder to copy the ippacket to the payload.
        unsigned char destmac[1024];   //parameter not used but defined inorder to copy the ippacket to the payload.
	short op;		       //parameter not used but defined inorder to copy the ippacket to the payload.
	char message[1024];
}ippacket;

struct arppacket			// creating the structure for arp packet
{
        unsigned char srcip[1024];
        unsigned char destip[1024];
	unsigned char sourcemac[1024];
        unsigned char destmac[1024];
	short op;			// op = 0 -> ARP Request and op = 1 ARP Response

}arppacket;

struct payload				// creating the structure for payload-- during ippacket sending the sourcemac, destmac, op parameter is empty.
{
        unsigned char sourceip[1024];
        unsigned char destinip[1024];

        unsigned char sourcemac[1024];
        unsigned char destmac[1024];
	short op;
	char message[1024];

}payload;

struct usermessage			// ethernet frame
{
        unsigned char sourcemac[512];
        unsigned char destmac[512];
        int type;			// type = 1 -> ARP Packet and type = 0 -> IPPacket
        struct payload pay; 
}usermessage;

void* g_start_timer(void* q)		// Timer function with threads to expire the selftable after a certain time
{
	int g = *((int *)q);
//	btable[g].timer = 60;

	while(btable[g].timer>=0)
	{
		sleep(6);
		btable[g].timer -- ;
		if(btable[g].timer == 0)
		{
			fprintf(stderr,"1 entry from the table timed out and it is %d\n",btable[g].sockfd);
			btable[g].sockfd = -2;
			strcpy(btable[g].macaddr,"");
			break;
		}
	}pthread_exit(NULL);
} 

int main(int args , char *arg[])
{
	pthread_t thread_id[50];	// thread parameters
	int pcrte[50];
	
	strcpy((char *)&ippacket.sourcemac,"0");	// setting the source and destmac of ippacket to 0 as it is not used.
        strcpy((char *)&ippacket.destmac,"0");

	int sock ,i,s,p,t,q,k,flag,f,tempfl;		// variable declarations
	int addrlen;
	int max_sock;
	int sockstation;
	int sendmessage,receivemessage;
	int stationsock[20];
	struct sockaddr_in bridge,address,temporary[20];
	char host[256];
	char *lanname,*ipaddress;
	int max_port,portnumber;
	addrlen = sizeof(bridge);
	char buffer[512];
	char *inpt;
	char words[10][256];

	sock = socket(AF_INET, SOCK_STREAM, 0);		// creating the socket
	fcntl(sock, F_SETFL, O_NONBLOCK);		// making the socket nonblocking

	bridge.sin_family	= AF_INET;
	bridge.sin_addr.s_addr	= INADDR_ANY;
	bridge.sin_port		= 0;

	fd_set readfds; 
	
	max_port = atoi(arg[2]);
	lanname = arg[1];
	
	for(t=0;t<20;t++)
	{
		btable[t].sockfd = -2;			// initialising the bridge table sockfd to -2
	}
	
	if(sock<0)
	{
		perror("Failed to create a socket");
		exit(1);
	}	
	if((bind(sock , (struct sockaddr*)&bridge , addrlen))<0)	// binding the ipaddress and port number
	{
		perror("Unable to bind the ipaddress and portnumber to the bridge");
		close(sock);
		exit(1);
	}
	if((getsockname(sock, (struct sockaddr *)&bridge , &addrlen))<0)
	{
		perror("Unable to get the output from getsockname");
		return -1;
	}
	if((gethostname(host,addrlen))<0)
	{
		perror("Unable to get the hostname");
		return -1;
	}
	fprintf (stderr,"Bridge has been binded with address : %s , port: %d\n",host,ntohs(bridge.sin_port));			
	
	ipaddress = host;
	portnumber= ntohs(bridge.sin_port); 
	
	FILE *fp1,*fp2;				// Creating the files for .addr and .port files
	char temp1[512],temp2[512];
	strcpy(temp1,"");
	strcpy(temp2,"");
	strcpy(temp1,lanname);
	strcpy(temp2,lanname);

	if(access(strcat(lanname,".addr"),F_OK)==-1)
	{
		fp1 = fopen(strncat(temp1,".addr",sizeof(temp1)+4),"w");
		fprintf(fp1,"%s",ipaddress);
		fclose(fp1);
	}
/*	else
		printf("LANNAME already exists");*/

	if(access(strcat(lanname,".port"),F_OK)==-1)
	{
		 fp2 = fopen(strncat(temp2,".port",sizeof(temp2)+4),"w");
		 fprintf(fp2,"%d",portnumber);
		 fclose(fp2);
	}
/*	else
		printf("LANNAME already exists");*/

	listen(sock,15);
 
	for(i=0 ; i<=max_port ; i++)		// initialising stationsock to -1 
	{
		stationsock[i] = -1;
	}

	do
	{
		FD_ZERO(&readfds);
		FD_SET(0,&readfds);
		FD_SET(sock , &readfds);
		if(sock<0)
		max_sock = 0;
		else
		max_sock = sock;
		for( i=0;i<max_port;i++)
		{
			if(stationsock[i]>=0)
			{
				FD_SET(stationsock[i],&readfds);
			}
			if(stationsock[i]>max_sock)
			{
				max_sock = stationsock[i];
			}
		}	
 		select (max_sock + 1, &readfds, NULL , NULL , NULL);
		
		if(FD_ISSET(0,&readfds))		// code to give the command to display selflearning table
                {       tempfl = 0;
                        fgets(buffer,sizeof(buffer),stdin);
                        inpt = strtok(buffer , "-");
                        while(inpt != NULL)
                        {
                             strcpy(words[tempfl],inpt);
                             tempfl++;
                             inpt = strtok(NULL , "-");
                        }
                        if(strcmp(words[0],"show")==0)
                        {
                             if(strcmp(words[1],"SELF\n")==0)
                             {
				    fprintf(stderr,"\nSelfLearning Table \n ");
                                    fprintf(stderr,"--------------------------\n");
                                    for(t=0;t<20;t++)
                                    {       
                                    if(btable[t].sockfd!=-2)
                                    {
                                             fprintf(stderr,"%d\t %s\n",btable[t].sockfd,btable[t].macaddr);
                                    }
                                    }
			      }
			}
		}

		if(FD_ISSET(sock,&readfds))
		{
			for(i=0;i<= max_port;i++)				//Accepting and Rejecting connection
			{
				if(stationsock[i]==-1)
				{
				if((stationsock[i] = accept(sock , (struct sockaddr *)&address , (socklen_t*)&addrlen))<0)
				{
					perror("Failed to accept the client connection");
					exit(1);
				}
				else
				{
					fcntl(stationsock[i], F_SETFL, O_NONBLOCK);	//making the station sock nonblocking
					if(i<max_port)
					{
					temporary[i] = address;					
					printf("Established connection to the station , socketfd is : %d , address is : %s , port is : %d\n" , stationsock[i],
					inet_ntoa(address.sin_addr) , ntohs (address.sin_port));
					strcpy(usermessage.pay.message,"ACCEPT");
					if((send(stationsock[i],(struct usermessage*)&usermessage,sizeof(usermessage),0))<0)
					{
						perror("Unable to send the success message");
					}
					else
					{
						printf("Success message has been sent to connected station with file descriptor %d\n",stationsock[i]);
						strcpy(usermessage.pay.message,"");
					}
					break;
					}
					else
					{
						strcpy(usermessage.pay.message,"REJECT");
                                                if((send(stationsock[i],(struct usermessage*)&usermessage,sizeof(usermessage),0))<0)
                                                {
                                                        perror("Unable to send the reject message");
                                                }
                                                else
                                                {
                                                        printf("Reject message has been sent to the station with the file descriptor %d\n",stationsock[i]);
                                                        close(stationsock[i]);
							stationsock[i]=-1;
                                                        strcpy(usermessage.pay.message,"");
                                                        FD_CLR(stationsock[i],&readfds);
						
                                                }                                           
					}

				}
				}
			}
		}
		
		for(i=0;i<max_port;i++)				//Receiving and Sending messages with stations
		{
			if(stationsock[i]>=0)
			{
				if(FD_ISSET(stationsock[i],&readfds))
				{
					receivemessage = recv(stationsock[i],(struct usermessage*)&usermessage,sizeof(usermessage),0);
				        p = i;
					if(receivemessage < 0)
					{ 
						perror("Failed to receive the message");
						close(stationsock[i]);
						exit(1);
					}
					else if(receivemessage == 0)
					{
						printf("Station which got disconnected is socket : %d , address : %s , port :%d\n",stationsock[i],inet_ntoa
						(temporary[i].sin_addr),ntohs(temporary[i].sin_port));
						close(stationsock[i]);
						stationsock[i] = -1;
						break;
					}
					else
					{							// Self Learning Functionality
						for(t=0;t<20;t++)
						{f=0;
							if(btable[t].sockfd == stationsock[i])
                                                        { 
								btable[t].timer = 30;
								pcrte[t] = pthread_create(&thread_id[t],NULL,g_start_timer,(void *)&t);
							}

						 	if(btable[t].sockfd==-2)
							{
						 		for(q=0;q<20;q++)
								{
									if(btable[q].sockfd == stationsock[i])
									{ 
										f=1;
										break;
									}
								}
							
								if(f==0)
								{
									btable[t].sockfd  = stationsock[i];
									strcpy(btable[t].macaddr,usermessage.sourcemac);
									btable[t].timer = 30;
									pcrte[t] = pthread_create(&thread_id[t],NULL,g_start_timer,(void *)&t); 
									if(pcrte[t]!=0)
									fprintf(stderr,"Error");
									break;
								}
						        }                                
						}
						
						flag = 0;
						for(t=0;t<20;t++)		// Unicasting messages
						{
							if(strcmp(btable[t].macaddr,usermessage.destmac)==0)
							{
								if(send(btable[t].sockfd,(struct usermessage*)&usermessage,sizeof(usermessage),0)<0)
								{
									perror("Unable to send the message1");
								}
								else
								{
									fprintf(stderr,"Unicast Message sent is to filedescriptor %d\n",btable[t].sockfd);
								        
								}
							flag = 1;
							break;
							}
						}			
						if(flag == 0)			// Broadcasting messages
						{
							for(t=0;t<max_port ;t++)
                                			{
                                				if (t==p)
                                				{
                                        				continue;
                                				}
                                				if (stationsock[t] >= 0)
                                				{
                                        				if((send( stationsock[t] ,(struct usermessage*)&usermessage , sizeof(usermessage),0 ))<0)
                                        				{
                                        					perror ("unable to send the message");
                                        					close (stationsock[t]);
                                        				}
                                        				else
                                        				{
                                        					printf("Broadcast message sent is to filedescriptor %d\n ",stationsock[t]);
										
                                        				}
                                				}
                                			}
						}				
					}
				}
			}	
		}
		
	}while(1);
	return 0;				 
}
