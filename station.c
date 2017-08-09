
#include<stdio.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>


struct iface				// defining the structure for interface file
{
	char stationname[20];
	unsigned char station_ip[512];
	unsigned char network_mask[1024];
	unsigned char mac[1024];
	char bridgename[512];
}iface[5];

struct hostname				// defining the structure for host file
{
	char stationname[20];
	unsigned char station_ip[512];
	unsigned char mac[1024];
	struct in_addr host_ip;
}hname[20];

struct routingtable			// defining the structure for routing table
{
	unsigned char dest_networkaddr[512];
	unsigned char nexthop_router[512];
	unsigned char subnetmask[512];
	char stationname[512];
	struct in_addr destnetadd , subntmak;

}rtable[20];

struct arpcache				// defining the structure for arp cache
{
	unsigned char ipaddress[512];
	unsigned char macaddress[512];
}arpcache[20];

struct ippacket				// defining the structure for ippacket
{
	unsigned char sourceip[1024];
	unsigned char destinip[1024];
	unsigned char sourcemac[1024];	// sourcemac, destmac,op parameter not used but defined inorder for proper copying of packet to payload 
        unsigned char destmac[1024];
	short op;			// op = 0 ->ARP Request and op = 1 -> ARP Response 
	char message[1024];
}ippacket;

struct arppacket			// defining the structure for arppacket
{
	unsigned char srcip[1024];
	unsigned char destip[1024];
	unsigned char sourcemac[1024];
      	unsigned char destmac[1024];
	short op;
}arppacket;

struct payload				// defining the structure for payload where sourcemac,destmac and op will not be active during ippacket sending 
{					// message will not be active during arppacket sending
        unsigned char sourceip[1024];
        unsigned char destinip[1024];

	unsigned char sourcemac[1024];
        unsigned char destmac[1024];
	short op;
	char message[1024];
}payload;

struct routemapping			// defining the structure for routing table(mapping between the router and interface)
{
	int socketfd;
	char bridgename[100];
}rtmapng[20];

struct pendingqueue			// defining the structure for pending queue
{
	unsigned char destip[512];
	struct ippacket ipkt;
}pqueue[100];

struct usermessage			// defining the structure for ethernet packet
{
	unsigned char sourcemac[512];
	unsigned char destmac[512];
	int type;			// type = 1 -> ARP Packet and type = 0 -> IPPacket
	struct payload pay ;
}usermessage,destinemessage;

int main(int args , char *arg[])
{
	strcpy((char *)&ippacket.sourcemac,"0");	//setting the source mac and destmac in ippacket to 0 as it is never used.
	strcpy((char *)&ippacket.destmac,"0");

	int stationsock;int routersock[5];struct in_addr tempo;unsigned char nexthop[512],interface[512];	// variable declarations
	int addrlen;int choice,routesock,count=0;
	int a,i,j,p,l,x,t,k,tempfl;int g = 0;
	unsigned char temp1[100];unsigned char test[1024];
	int receivemessage,sendmessage;
	struct sockaddr_in bridge;
	struct hostent *hp;
	struct in_addr next1hop ;
	char next_hop[INET_ADDRSTRLEN];
	addrlen = sizeof(bridge);
	fd_set readfds,readf;
	int max_sock,max_sk;
	char strt[512],data[1024],buffer[1024],tempst[500];
	char *inpt;
	char tempcache[512];
	char words[10][256];

	FILE *f1,*f2,*f3,*f4,*f5;		// creating file pointers to copy the details from interface, host and routing files to the structures defined.
	f1 = fopen(arg[2],"r");
	f2 = fopen(arg[3],"r");
	f3 = fopen(arg[4],"r"); 
	char line[256] ;
	//strcpy(line,"");
	k=0;
	
	for(i=0;i<5;i++)
	{
		rtmapng[i].socketfd = -1;	
	}
	fprintf(stderr,"%s","Interface file details\n");
	fprintf(stderr,"%s","------------------------\n");
	while(fgets(line,sizeof(line),f1))	// copying the interface file details to the structure and printing it
	{
	sscanf(line,"%s %s %s %s %s" ,(char *)&iface[k].stationname,(char *)&iface[k].station_ip,(char *)&iface[k].network_mask,(char *)&iface[k].mac ,
	(char *)&iface[k].bridgename);
	
	fprintf(stderr,"%s\t%s\t%s\t%s\t%s\t\n",iface[k].stationname,iface[k].station_ip,iface[k].network_mask,iface[k].mac,iface[k].bridgename);
	k++;
	}int w = k;
	memset(line,0,sizeof(line));

	fprintf(stderr,"%s","\nRouting table details   \n");
        fprintf(stderr,"%s","------------------------\n");

	i= 0; 
	while(fgets(line,sizeof(line),f2))	// copying the routing table file details and printing it
	{
        sscanf(line,"%s %s %s %s" ,(char *)&rtable[i].dest_networkaddr,(char *)&rtable[i].nexthop_router,(char *)&rtable[i].subnetmask,(char *)&rtable[i].stationname);
        fprintf(stderr,"%s\t %s\t %s\t %s\t \n",rtable[i].dest_networkaddr,rtable[i].nexthop_router,rtable[i].subnetmask,rtable[i].stationname);
	
	if(inet_pton(AF_INET , rtable[i].dest_networkaddr ,&rtable[i].destnetadd) == 0)
		perror("Invalid address"); 
	
	if(inet_pton(AF_INET , rtable[i].subnetmask ,&rtable[i].subntmak) == 0)
                perror("Invalid address"); 	
	i++;
	}int z = i;
	
	j=0;
	memset(line,0,sizeof(line));
	fprintf(stderr,"%s","\nHost table details   \n");
        fprintf(stderr,"%s","------------------------\n");

	while(fgets(line,sizeof(line),f3))	// copying the hostname file details and printing it.
        {
	sscanf(line,"%s %s" ,(char *)&hname[j].stationname,(char *)&hname[j].station_ip);
	memset(line,0,sizeof(line));
	if(inet_pton(AF_INET , hname[j].station_ip ,&hname[j].host_ip) == 0)
                perror("Invalid address"); 
	fprintf(stderr,"%s\t%s\t\n",hname[j].stationname,hname[j].station_ip);
	j++;

	}int s = j;
	fclose(f3);
	fprintf(stderr,"\n");
	strcpy(strt,arg[1]);
					
        /*--------------------------------------------------station part starts here----------------------------------------------------*/
	if(strcmp(strt,"-no")==0)					
        {
	stationsock = socket(AF_INET,SOCK_STREAM,0);
        if(stationsock == -1)
        {
                perror("Unable to create a socket for station/router");
                exit(1);
        }
        
        bridge.sin_family = AF_INET;
	char temp11[512],temp22[512],temp2[512];int temp;
	strcpy(temp11,iface[0].bridgename);
	strcpy(temp2,iface[0].bridgename);
	f4 = fopen(strcat(temp11,".addr"),"r");
	f5 = fopen(strcat(temp2,".port"),"r");
	while(fgets(line,sizeof(line),f4))		// command to read the .addr and .port files to connect to the bridge
        {
        sscanf(line,"%s" ,temp22);
        }
	while(fgets(line,sizeof(line),f5))
        {
        sscanf(line,"%d" ,&temp);
        }
	hp = gethostbyname(temp22); 

        if(hp==0)
        {
                perror("Unable to get the bridge host name");
        }
        
        bridge.sin_port = htons(temp);    
        memcpy(&bridge.sin_addr , hp->h_addr , hp->h_length);

	if((connect(stationsock , (struct sockaddr *)&bridge , addrlen))<0)
	{
		perror("Unable to connect to the bridge");
		exit(1);	
	}
	for(i=0;i<s;i++)
        {
                strcpy(arpcache[i].macaddress,"0");
        }
	for(i=0;i<s;i++)
        {
                strcpy(arpcache[i].ipaddress,hname[i].station_ip) ;  //forming the arp cache
                                
                if(strcmp(arpcache[i].ipaddress,iface[0].station_ip)==0)
                {
                      strcpy(arpcache[i].macaddress,iface[0].mac);
                }
        }

	do
	{
		FD_ZERO(&readfds);
		FD_SET(0,&readfds);
		max_sock = stationsock;
		FD_SET(stationsock,&readfds);
		memset(&buffer,'0',sizeof(buffer));

		if(stationsock<0)
		{
			max_sock = 0;
		}

		select(max_sock+1,&readfds,NULL,NULL,NULL);

		strcpy(tempst,"");

		if(FD_ISSET(0,&readfds))
		{	tempfl = 0;
			fgets(buffer,sizeof(buffer),stdin);
			inpt = strtok(buffer , "-");
			while(inpt != NULL)			// command to send the messages
			{
				strcpy(words[tempfl],inpt);
				tempfl++;
				inpt = strtok(NULL , "-");
			}
				if(strcmp(words[0],"send")==0)
				{
				strcpy(ippacket.message,words[2]);
				
			/*creating ip packet*/
			strcpy(tempst,words[1]);
			strcpy(usermessage.sourcemac,iface[0].mac);
			strcpy(ippacket.sourceip,iface[0].station_ip);		

			for(i=0;i<s;i++)
			{
				if(strcmp(hname[i].stationname,tempst)==0)
                                {
					strcpy(ippacket.destinip,hname[i].station_ip);
                                	tempo = hname[i].host_ip;
					strcpy(test,hname[i].station_ip);
				}
			}
			
			choice  = 0 ;			// checking whether destination is in same network or not
			for(i=0;i<z;i++)
			{	
				if((tempo.s_addr & rtable[i].subntmak.s_addr) == rtable[i].destnetadd.s_addr)
				{
					if(strcmp(rtable[i].nexthop_router,"0.0.0.0")==0)
					{
						choice = 1;
					}
					else
					{
						strcpy(nexthop,rtable[i].nexthop_router);
					}
					break;
				}
			}

			for(i=0;i<s;i++)		// choice = 1 is same network and choice = 0 is different network	
			{
				if(choice == 1)
				{
				if(strcmp(ippacket.destinip,arpcache[i].ipaddress)==0)
				{
					if(strcmp(arpcache[i].macaddress,"0")==0)	//if mac address not found in cache creating arppacket
					{   //   arp packet creation
						usermessage.type = 1;
						strcpy(usermessage.destmac,"FF:FF:FF:FF:FF:FF"); 
						strcpy(arppacket.sourcemac,iface[0].mac);
						strcpy(arppacket.srcip,iface[0].station_ip); 
						strcpy(arppacket.destip,ippacket.destinip);
						arppacket.op = 0;
						memset(usermessage.pay.message,0,sizeof(usermessage.pay.message));
						for(k=0;k<20;k++)
						{
							memcpy(&pqueue[k].ipkt,&ippacket,sizeof(ippacket));	// storing ippacket in pending queue
							strcpy(pqueue[k].destip,arppacket.destip);
						}
					}
					else
					{
						usermessage.type = 0;			//if found upadting the destinaton mac
						strcpy(usermessage.destmac,arpcache[i].macaddress);
					}
				break;
				}
				}
				else
				{
				if(strcmp(nexthop,arpcache[i].ipaddress)==0)
                                {
                                        if(strcmp(arpcache[i].macaddress,"0")==0)	//if nexthop and macaddress not found creating arp packet
                                        {   //   arp packet creation
                                                usermessage.type = 1;
                                                strcpy(usermessage.destmac,"FF:FF:FF:FF:FF:FF"); 
                                                strcpy(arppacket.sourcemac,iface[0].mac);
                                                strcpy(arppacket.srcip,iface[0].station_ip); 
                                                strcpy(arppacket.destip,nexthop);
						arppacket.op = 0;
						memset(usermessage.pay.message,0,sizeof(usermessage.pay.message));
						for(k=0;k<20;k++)
                                                {
							if(strcmp(pqueue[k].destip,"")==0)
							{
                                                        memcpy(&pqueue[k].ipkt,&ippacket,sizeof(ippacket));	// storing ippacket in pending queue
                                                        strcpy(pqueue[k].destip,arppacket.destip);
							break;
							}
                                                }
                                        }
                                        else
                                        {
                                                usermessage.type = 0;			//if nexthop and macaddress found updating the destination amc address
                                                strcpy(usermessage.destmac,arpcache[i].macaddress);
                                        }
                                break;
                                }
				}
			}
					if(usermessage.type==1)
					{
						memcpy(&usermessage.pay,&arppacket,sizeof(arppacket));
						fprintf(stderr,"sending arprequest");
					}
					else
					{
						memcpy(&usermessage.pay,&ippacket,sizeof(ippacket));
						fprintf(stderr,"sending ippacket with the ethernet frame\n");
					}
	
			sendmessage = send(stationsock,(struct usermessage*)&usermessage,sizeof(usermessage),0);	//sending the message with either ARP or IP
			if(sendmessage<0)
			{
				perror("Unable to send data");
				close(stationsock);
				exit(1);
			}
			else
			{
				continue;
			}
				}
				else if(strcmp(words[0],"show")==0)			//if command is show then code to show the ARP cache.
				{
					if(strcmp(words[1],"ARP\n")==0)
					{
					fprintf(stderr,"\nArp Cache at the station\n");
                                        fprintf(stderr,"------------------------------------------------\n");           
                                        for(i=0;i<s;i++)
                                        {
                                                fprintf(stderr,"%s\t %s\n",arpcache[i].ipaddress,arpcache[i].macaddress);
                                        }
					}
				}					
		}
			
		if(FD_ISSET(stationsock,&readfds))			// Station receiving messages
		{
			receivemessage = recv(stationsock,(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
   
			if((receivemessage) <0)
			{
				perror("Unable to receive the message");
				close(stationsock);
				exit(1);
			}
			else if(receivemessage ==0)
			{
				close(stationsock);
				fprintf(stderr,"The bridge got disconnected\n");
				exit(1);
				continue;
			}
			else
			{
				if(strcmp(destinemessage.pay.message,"ACCEPT")==0)	// Printing accept and reject message from the bridge
				{
					fprintf(stderr , "Connection to station has been accepted by the bridge\n");
				}
				else if(strcmp(destinemessage.pay.message,"REJECT")==0)
				{
					fprintf(stderr , "Connection to station has been rejected by the bridge\n");
					close(stationsock);
					exit(1);
				}
				
				if(strcmp(destinemessage.destmac,iface[0].mac)==0) 
                                {       
                                    if(destinemessage.type==0)				// displaying the message
				    {
				    	fprintf(stderr,"Message received from the bridge is %s\n",destinemessage.pay.message);
				    }
				    else if(destinemessage.pay.op == 1)
				    {  	/*ARP Response Received*/
					fprintf(stderr,"\nARP response received and sending ethernet frame with macaddress\n");
					strcpy(destinemessage.sourcemac,iface[0].mac);
                                        strcpy(destinemessage.destmac,destinemessage.pay.sourcemac);
					destinemessage.type = 0;
					strcpy(tempcache,destinemessage.pay.sourcemac);
					memset(destinemessage.pay.sourcemac,0,sizeof(destinemessage.pay.sourcemac));
					memset(destinemessage.pay.destmac,0,sizeof(destinemessage.pay.destmac));	
					destinemessage.pay.op = -1;
					for(k=0;k<20;k++)
					{
					if(strcmp(pqueue[k].destip,destinemessage.pay.destinip)==0)
					{									//encapsulating the ippacket with ethernet frame
					memcpy(&destinemessage.pay,&pqueue[k].ipkt,sizeof(pqueue[k].ipkt));
					strcpy(pqueue[k].destip,"");						// making the pending queue null
					strcpy(pqueue[k].ipkt.sourceip,"");
					strcpy(pqueue[k].ipkt.destinip,"");
					strcpy(pqueue[k].ipkt.message,"");
					break;
					}
					}
					for(i=0;i<s;i++)
					{ 
					if(choice == 1)					// storing the destination mac address in the cache
					{
					if(strcmp(ippacket.destinip,arpcache[i].ipaddress)==0)
                                	{
                                        	strcpy(arpcache[i].macaddress,tempcache);     
                                	}
					}
					else 
					{
					if(strcmp(nexthop,arpcache[i].ipaddress)==0)
                                        {
                                                strcpy(arpcache[i].macaddress,tempcache);     
                                        }

					}
					}
					// sending the ethernet frame with ippacket encapsulated
                                        sendmessage = send(stationsock,(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
				    }
                                }
				else if(strcmp(destinemessage.destmac,"FF:FF:FF:FF:FF:FF")==0)
				{
					if(strcmp(destinemessage.pay.destinip,iface[0].station_ip)==0) // for arp response 
                                	{
						strcpy(temp1,"");
                                        	strcpy(destinemessage.pay.sourcemac,iface[0].mac);
                                        	strcpy(temp1,destinemessage.sourcemac);
						strcpy(destinemessage.pay.destmac,temp1);
                                        	strcpy(destinemessage.sourcemac,iface[0].mac);
						memset(destinemessage.destmac,0,sizeof(destinemessage.destmac));
                                        	strcpy(destinemessage.destmac,temp1);
						memset(destinemessage.pay.message,0,sizeof(destinemessage.pay.message));
						destinemessage.pay.op = 1;
						for(i=0;i<s;i++)
                                        	{
						 	if(strcmp(destinemessage.pay.sourceip,arpcache[i].ipaddress)==0)
                                        		{
                                                		strcpy(arpcache[i].macaddress,temp1);     
                                        		}
                                        	}
 						fprintf(stderr,"\nsending the arp response\n");
                                        	sendmessage = send(stationsock,(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
                                        	if(sendmessage<0)
                                        	{
                                                perror("Unable to send");
						close(stationsock);
						exit(1);
                                        	}
                                	}
				}
			}
		}
	}
	while(1);		
	}
	/*----------------------------------Router starts here--------------------------*/
	else if(strcmp(strt,"-route")==0)
	{
		for(a=0;a<2;a++)
		{
			routersock[a] = -1;
		}
		
		for(i=0;i<s;i++)             
                        {    
                                strcpy(arpcache[i].macaddress,"0");
                        }
                
                        for(i=0;i<s;i++)
                        {
                                strcpy(arpcache[i].ipaddress,hname[i].station_ip) ;  //forming the arp cache
                                
                                for(l=0;l<w;l++)
                		{
				if(strcmp(arpcache[i].ipaddress,iface[l].station_ip)==0)
                                {
                                        strcpy(arpcache[i].macaddress,iface[l].mac);
					break;
                                }
				}
			}		

		for(a=0;a<2;a++)					// creating the router socket
                {
		routersock[a] = socket(AF_INET,SOCK_STREAM,0);
        	if(routersock[a] == -1)
        	{
                	perror("Unable to create a socket for station/router");
                	exit(1);
        	}
        
        	bridge.sin_family = AF_INET;
        	char temp11[512],temp22[512],temp2[512];int temp;
       
		strcpy(temp11,iface[a].bridgename);			//connecting the router to bridge using .addr and .port files
        	strcpy(temp2,iface[a].bridgename);
        	f4 = fopen(strcat(temp11,".addr"),"r");
        	f5 = fopen(strcat(temp2,".port"),"r");
        	while(fgets(line,sizeof(line),f4))
        	{
        		sscanf(line,"%s" ,temp22);
        	}
        	while(fgets(line,sizeof(line),f5))
        	{
        		sscanf(line,"%d" ,&temp);
        	}
        		hp = gethostbyname(temp22);
			if(hp==0)
        		{
                		perror("Unable to get the bridge host name");
        		}
          
        		bridge.sin_port = htons(temp);    
        		memcpy(&bridge.sin_addr , hp->h_addr , hp->h_length);

        		if((connect(routersock[a] , (struct sockaddr *)&bridge , addrlen))<0)
        		{
                		perror("Connection rejected by bridge as it exceeded the number of ports/or some of the bridges which has to be connected to this router are not active");
                		exit(1);         
        		} 
		
				for(i=0;i<5;i++)
				{
					if(rtmapng[i].socketfd == -1)		// storing the mapping between the router and interface
					{
						rtmapng[i].socketfd = routersock[a];
						strcpy(rtmapng[i].bridgename,iface[a].bridgename);
					//	fprintf(stderr,"socket %d\t bridgename %s\t \n",rtmapng[i].socketfd,rtmapng[i].bridgename);
						break;
					}
				}
		}		
			
          		do
        		{ 
				FD_ZERO(&readfds);
				FD_SET(0,&readfds);  
				for(a=0;a<2;a++)
 				{
				FD_SET(routersock[a],&readfds);
				if(routersock[a]<0)
				max_sock = 0;
				else
				max_sock = routersock[a];
                                if(a>0)
                                {
                                if(routersock[a]>routersock[a-1])
                                {
                                        max_sock = routersock[a];
                                }
				}
				}
     
				select(max_sock+1,&readfds,NULL,NULL,NULL);
                		if(FD_ISSET(0,&readfds))			// command to show the arp cache for the router
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
                                                if(strcmp(words[1],"ARP RT\n")==0)
                                                {
                                                        fprintf(stderr,"\nArp Cache for the router\n");
                                                        fprintf(stderr,"------------------------------------------------\n");           
                                                                for(i=0;i<s;i++)
                                                                {
                                                                        fprintf(stderr,"%s\t %s\n",arpcache[i].ipaddress,arpcache[i].macaddress);
                                                                }
                                                }
                                        }     
				}  

				for(a=0;a<2;a++)
                              	{       
				if(FD_ISSET(routersock[a],&readfds))	// receiving messages
                		{
                        			receivemessage = recv(routersock[a],(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
   				
                        			if((receivemessage) <0)
                        			{
                                			perror("Unable to receive the message");
                                			close(routersock[a]);
                               			 	exit(1);
                        			}
                        			else if(receivemessage ==0)
 						{
							close(routersock[a]);
							fprintf(stderr,"The bridge got disconnected\n");
							exit(1);
                                			continue;
                        			}
                        			else		// receiving accept and reject messages from the bridge
                        			{
                                		if(strcmp(destinemessage.pay.message,"ACCEPT")==0)
                                		{
                                        		fprintf(stderr , "Connection to router has been accepted by the bridge\n");
                                		//	fprintf(stderr , "Router sock value %d\n ",routersock[a]);
						}
                                		else if(strcmp(destinemessage.pay.message,"REJECT")==0)
                                		{
                                        		fprintf(stderr , "Connection to router has been rejected by the bridge\n");
							close(routersock[a]);
							exit(1);
                                		}
					
						if(strcmp(destinemessage.destmac,iface[a].mac)==0) 
                                		{       
                                    			if(destinemessage.type==0)	// if ippacket received in ethernet frame
                                    			{
								memcpy(&ippacket,&destinemessage.pay,sizeof(ippacket));
                                       
                             					for(i=0;i<s;i++)
                        					{
                                				if(strcmp(destinemessage.pay.destinip,hname[i].station_ip)==0)
                                				{
                                        				tempo = hname[i].host_ip;
                                				}
                        					}

                        					choice  = 0 ;		// if destination ip is in same network or not(choice = 1 in same network
                        					for(i=0;i<z;i++)	// choice = 0 in different network
                        					{
                                				if((tempo.s_addr & rtable[i].subntmak.s_addr) == rtable[i].destnetadd.s_addr)
                                				{
                                        			if(strcmp(rtable[i].nexthop_router,"0.0.0.0")==0)
                                        			{
                                                			choice = 1;
                                       				}
                                        			else
								{
                                                		strcpy(nexthop,rtable[i].nexthop_router);
                                        			}
								for(p=0;p<5;p++)
								{
								if(strstr(rtable[i].stationname,rtmapng[p].bridgename)!=NULL)
								{
									routesock = rtmapng[p].socketfd;
									strcpy(interface,rtmapng[p].bridgename);
									break;
								}
								}
                                        			break;
                                				}
                        					}
								
								for(i=0;i<s;i++)	//if no mac in cache creating arp request else sending ippacket
                        					{
                                					if(choice == 1)
                               	 					{
                               	 					if(strcmp(destinemessage.pay.destinip,arpcache[i].ipaddress)==0)
                                					{
										for(p=0;p<2;p++)
										{
                                        					if(strcmp(arpcache[i].macaddress,"0")==0)
                                        					{   //   arp packet creation
                                                					destinemessage.type = 1;
											strcpy(destinemessage.destmac,"FF:FF:FF:FF:FF:FF");
											memset(destinemessage.pay.message,0,sizeof(destinemessage.pay.message)); 
											
												if(strcmp(interface,iface[p].bridgename)==0)
												{
													strcpy(destinemessage.sourcemac,iface[p].mac);
													strcpy(arppacket.sourcemac,iface[p].mac);
                                                							strcpy(arppacket.srcip,iface[p].station_ip); 
                                                							strcpy(arppacket.destip,destinemessage.pay.destinip);
													arppacket.op = 0;
													for(k=0;k<20;k++)
                                                							{
                                                        						if(strcmp(pqueue[k].destip,"")==0)
                                                        						{
                                                        						memcpy(&pqueue[k].ipkt,&ippacket,sizeof(ippacket));
                                                        						strcpy(pqueue[k].destip,arppacket.destip);
                                                        						break;
                                                        						}
                                                							}

													break;
												}
                                        					}
                                        					else
                                        					{
											destinemessage.type = 0;
                                                                                        strcpy(destinemessage.destmac,arpcache[i].macaddress);

											if(strcmp(interface,iface[p].bridgename)==0)
                                                                                        {
												strcpy(destinemessage.sourcemac,iface[p].mac);
												break;
											}
                                        					}
										}
                                					break;
                                					}
                                					}
                                					else
                                					{
                                						if(strcmp(nexthop,arpcache[i].ipaddress)==0) 
                               							{
										for(p=0;p<2;p++)
                                                                                {
                                        						if(strcmp(arpcache[i].macaddress,"0")==0)
                                        						{   //   arp packet creation
                                                						destinemessage.type = 1;
												memset(destinemessage.pay.message,0,sizeof(destinemessage.pay.message));
												strcpy(destinemessage.destmac,"FF:FF:FF:FF:FF:FF");
								
                                                                                		if(strcmp(interface,iface[p].bridgename)==0)
										        	{
                                                                                        	strcpy(destinemessage.sourcemac,iface[p].mac);
                                                                                        	strcpy(arppacket.sourcemac,iface[p].mac);
                                                                                        	strcpy(arppacket.srcip,iface[p].station_ip); 
                                                                                        	strcpy(arppacket.destip,nexthop);
												arppacket.op = 0;
												for(k=0;k<20;k++)
                                                						{
                                                        					if(strcmp(pqueue[k].destip,"")==0)
                                                        					{
                                                        					memcpy(&pqueue[k].ipkt,&ippacket,sizeof(ippacket));
                                                        					strcpy(pqueue[k].destip,arppacket.destip);
                                                        					break;
                                                        					}
                                                						}

												break;
                                                                                        	}
                                        						}
                                        						else
                                        						{
												destinemessage.type = 0;               
                                                                                                strcpy(destinemessage.destmac,arpcache[i].macaddress);
                                                                                        	if(strcmp(interface,iface[p].bridgename)==0)
                                                                                                {												
                                                                                        	strcpy(destinemessage.sourcemac,iface[p].mac);
                                                                                        	break;
												}
                                        						}
										} 
                                						break;
               	                						}
									}							
								}
							if(destinemessage.type==1)
                                        		{
								fprintf(stderr,"\nsending the arp request\n");
                                                		memcpy(&destinemessage.pay,&arppacket,sizeof(arppacket));
                                        		}
                                        		else
                                       	 		{
								fprintf(stderr,"\nsending the ethernet frame with ippacket\n");
                                                		memcpy(&destinemessage.pay,&ippacket,sizeof(ippacket));
                                        		}
								// sending message with ARP request or ippacket based on the condition
								sendmessage = send(routesock,(struct destinemessage*)&destinemessage,sizeof(destinemessage),0);        
                        					if(sendmessage<0)
                        					{
                                				perror("Unable to send data");
                                				close(routesock);
                                				exit(1);
                        					}
							}

                                    			else if(destinemessage.pay.op == 1)
                                    			{
								// ARP Response received
                                        			strcpy(destinemessage.sourcemac,iface[a].mac);
                                        			strcpy(destinemessage.destmac,destinemessage.pay.sourcemac);
                                        			destinemessage.type = 0;
								strcpy(tempcache,destinemessage.pay.sourcemac);
								memset(destinemessage.pay.sourcemac,0,sizeof(destinemessage.pay.sourcemac));
								memset(destinemessage.pay.destmac,0,sizeof(destinemessage.pay.sourcemac));
								destinemessage.pay.op = -1;
								for(i=0;i<s;i++)
                                                                {
                                                                if(strcmp(destinemessage.pay.destinip,arpcache[i].ipaddress)==0)
                                                                {
                                                                        strcpy(arpcache[i].macaddress,tempcache);     
                                                                }
                                                                }

								for(k=0;k<20;k++)	// emptying the pending queue
                                        			{
                                        			if(strcmp(pqueue[k].destip,destinemessage.pay.destinip)==0)
                                        			{
                                        			memcpy(&destinemessage.pay,&pqueue[k].ipkt,sizeof(pqueue[k].ipkt));
                                        			strcpy(pqueue[k].destip,"");
                                        			strcpy(pqueue[k].ipkt.sourceip,"");
                                        			strcpy(pqueue[k].ipkt.destinip,"");
                                        			strcpy(pqueue[k].ipkt.message,"");
                                        			break;
                                        			}
                                        			}

								fprintf(stderr,"\nArp response received and Sending the Ippacket\n");
                                        			sendmessage = send(routersock[a],(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
                                    			}
                                		}
                                		else if(strcmp(destinemessage.destmac,"FF:FF:FF:FF:FF:FF")==0)
 						{
                                        		if(strcmp(destinemessage.pay.destinip,iface[a].station_ip)==0) // for arp response 
                                        		{
                                                		strcpy(temp1,"");
                                                		strcpy(destinemessage.pay.sourcemac,iface[a].mac);
                                                		strcpy(temp1,destinemessage.sourcemac);
                                                		strcpy(destinemessage.sourcemac,iface[a].mac);
								memset(destinemessage.destmac,0,sizeof(destinemessage.destmac));
								memset(destinemessage.pay.message,0,sizeof(destinemessage.pay.message));
                                                		strcpy(destinemessage.destmac,temp1);
								destinemessage.pay.op = 1;
								for(i=0;i<s;i++)
                                                		{
                                                        		if(strcmp(destinemessage.pay.sourceip,arpcache[i].ipaddress)==0)
                                                        		{
                                                                		strcpy(arpcache[i].macaddress,temp1);     
                                                        		}
                                                		}
								fprintf(stderr,"arp response sent");
                                                		sendmessage = send(routersock[a],(struct usermessage*)&destinemessage,sizeof(destinemessage),0);
                                                		if(sendmessage<0)
                                                		{
                                                			perror("Unable to send");
                                                			close(routersock[a]);
                                                			exit(1);
                                                		}
 							}	
                                		}
                        		}
                		}
        		}
		}  while(1);
	    }         
return 0;
}
