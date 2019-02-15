/******************************************************************************************************
OWNER: SHASHANK KAMATH KALASA MOHANDAS
UIN: 627003580

*******************************************************************************************************/

#include<string.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<inttypes.h>
#include<signal.h>

#define BACKLOG 10

#define JOIN 2
#define FWD 3
#define SEND 4
#define NAK 5
#define OFFLINE 6
#define ACK 7
#define ONLINE 8
#define IDLE 9

//Declaring the variables
char *PORT;
char s[INET6_ADDRSTRLEN];
fd_set master;
fd_set read_fds;
int maxfd;
int socketfd, newsocketfd, flag, numbytes, yes=1;
int rcv,length=1;
int i, j, z,b,k=1;
int client_count=0;
struct addrinfo addressinfo, *servicelist, *loopvariable;
struct sockaddr_storage str_addr;
socklen_t addr_size;
char buffer[800];
char buffer_send[600];
char buffer_message_send[560];
char buffer_message_recv[512];
char buffer_username[16];
char usernames[100][16];
char list_name[500];
char client_exit[200];
struct sbcp_msg message_recv,message_send;
struct sbcp_attribute attribute_recv,attribute_send;
int16_t packetsize;

struct sbcp_attribute
{
	int16_t type;
	int16_t length;
	char* payload;
}attribute_recv,attribute_send;

struct sbcp_msg
{
	int8_t vrsn;
	int8_t type;
	int16_t length;
	struct sbcp_attribute* attribute;
}message_recv,message_send;

//Packing and Unpacking functions are taken from Beej's Guide to Network Programming.
void packi16(char *buf, unsigned int i)
{
*buf++ = i>>8; 
*buf++ = i;
}

int32_t pack(char *buf, char *format, ...)
{
    va_list ap;
    int16_t h;
	int8_t c;
    char *s;
    int32_t size = 0; 
	int32_t len;
    va_start(ap, format);
    for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				size += 2;
				h = (int16_t)va_arg(ap, int); 
				packi16(buf, h);
				buf += 2;
				break;
			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int); 
				*buf++ = (c>>0)&0xff;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = strlen(s);
				size += len + 2;
				packi16(buf, len);
				buf += 2;
				memcpy(buf, s, len);
				buf += len;
				break;
		}
	}
	va_end(ap);
	return size;
}

unsigned int unpacki16(char *buf)
{
	return (buf[0]<<8) | buf[1];
}

void unpack(char *buf, char *format, ...)
{
	va_list ap;
	int16_t *h;
	int8_t *c;
	char *s;
	int32_t len, count, maxstrlen=0;
	va_start(ap, format);
	for(; *format != '\0'; format++) {
		switch(*format) {
            case 'h': // 16-bit
				h = va_arg(ap, int16_t*);
				*h = unpacki16(buf);
				buf += 2;
				break;
			case 'c': // 8-bit
				c = va_arg(ap, int8_t*);
				*c = *buf++;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = unpacki16(buf);
				buf += 2;
				if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
				else count = len;
				memcpy(s, buf, count);
				s[count] = '\0';
				buf += len;
				break;
			default:
				if (isdigit(*format)) { 
					maxstrlen = maxstrlen * 10 + (*format-'0');
				}
		}
		if (!isdigit(*format)) maxstrlen = 0;
	}
	va_end(ap);
}

void *getaddress(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
	{
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *getport(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
	{
        return &(((struct sockaddr_in*)sa)->sin_port);
    }
		return &(((struct sockaddr_in6*)sa)->sin6_port);
}

// TO send the full packet data if not fully sent
int sendall(int socket_id, char *buf, int length)
{
int cnt = 0; 
int bytesleft = length; 
int bytessent;
while(cnt < length)
	{
		bytessent = send(socket_id, buf+cnt, bytesleft, 0);
		if (bytessent == -1) 
		{	 
			break; 
		}
		cnt = cnt + bytessent;	
		bytesleft = bytesleft - bytessent;
	}
length = cnt; 
return bytessent==-1?-1:0; // return -1 on failure, 0 on success
}

void sighandler(int signum)
{
	sprintf(buffer_message_send,"Server Terminated");
	attribute_send.payload=buffer_message_send;
	attribute_send.type=1;
	attribute_send.length=36;//32+4
	message_send.vrsn='3';
	message_send.type=NAK;
	message_send.length=40;
	message_send.attribute=&attribute_send;
	packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,buffer_message_send);
	for(z=4; z<=maxfd; z++)
	{
		if(sendall(z, buffer_send, packetsize)==-1)
		{
			perror("Server: Send Error during client exit \n");
		}
	}	
	exit(0);
}

int main(int argc, char *argv[])
{	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	int max_clients=atoi(argv[3]);
	if(argc !=4)
	{
		printf("Server: Excess Arguments Passed /n");
		exit(1);
	}
	
	memset(&addressinfo,0,sizeof (addressinfo)); // Making the addressinfo struct zero
	addressinfo.ai_family = AF_UNSPEC;// Not defining whether the connection is IPv4 or IPv6
	addressinfo.ai_socktype = SOCK_STREAM;
	addressinfo.ai_flags = AI_PASSIVE;
	
	if ((flag = getaddrinfo(argv[1], argv[2], &addressinfo, &servicelist)) != 0) 
	{
		printf("GetAddrInfo Error");
		exit(1);
	}
	printf("Server: Done with getaddrinfo \n");
	// Traversing the linked list for creating the socket
	for(loopvariable = servicelist; loopvariable != NULL; loopvariable = (loopvariable -> ai_next ))
	{
		if((socketfd = socket(loopvariable -> ai_family, loopvariable -> ai_socktype, loopvariable -> ai_protocol)) ==  -1 )
		{
			printf("Server: Socket Created.\n");
			continue;
		}
		if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==  -1) 
		{
			perror("Server: SetSockOpt.\n");
			exit(1);
		}
		//Binding the socket
		if (bind(socketfd, loopvariable->ai_addr, loopvariable->ai_addrlen) ==  -1)	
		{
			close(socketfd);
			perror("Server: Bind Error.\n");
			continue;
		}
		break;
	}
	// Freeing the linked  list
	freeaddrinfo(servicelist); 

	if (loopvariable == NULL) 
	{
		printf("Server: Failed to bind.\n");
		exit(1);
	}
	
	// Listen to the connection
	if (listen(socketfd, BACKLOG) ==  -1) 
	{
		perror("Server: Listen Error");
		exit(0);
	}
	printf("Server: Listening in progress \n");
	
	FD_SET(socketfd,&master);
	maxfd=socketfd;
	
	while(1)
	{
		read_fds=master;
		if(select(maxfd+1,&read_fds,NULL,NULL,NULL) == -1)
		{
			perror("Server: Select Error");
			exit(1);
		}
		
		for(i=0; i<=maxfd; i++)
		{
			if(FD_ISSET(i,&read_fds))
			{
				signal(SIGINT, sighandler);//checking for CTRL+C input
				if(i==socketfd)//Checking for a new connection
				{
						addr_size = sizeof str_addr;
						newsocketfd = accept(socketfd, (struct sockaddr *)&str_addr, &addr_size);
						//printf("Accepted new connection: \n");
						if (newsocketfd == -1)
						{
							perror("Server: Accept Error");
							exit(1);
						}
						else
						{
							FD_SET(newsocketfd,&master);
							if(newsocketfd>maxfd)
							{
								maxfd=newsocketfd;
							}
							client_count=client_count+1;
						}
				}
				else
				{
					
					if((rcv=recv(i,buffer,600,0))<=0)
					{
						if(rcv==0)//If client exits unceremonously
						{
							printf("Client %s has disconnected\n",usernames[i]);
							attribute_send.payload=usernames[i];
							attribute_send.type=2;
    						attribute_send.length=20;//2+2+16
							message_send.vrsn='3';
							message_send.type=OFFLINE;
							message_send.length=24; 
							message_send.attribute=&attribute_send;
							packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,attribute_send.payload);
							for(j = 0; j <= maxfd; j++) 
							{
								if (FD_ISSET(j, &master)) 
								{
									if (j != socketfd && j != i) 
									{
										if(sendall(j, buffer_send, packetsize)==-1)
										{
											perror("Server: Send Error during client exit \n");
										}
									}	
								}
							}
							usernames[i][0]='\0';
						}
						else
						{
							perror("Server: Receive Error \n");
						}
						client_count=client_count-1;
						close(i);
						FD_CLR(i,&master);
					}
					else
					{	
						unpack(buffer,"cchhh",&message_recv.vrsn,&message_recv.type,&message_recv.length,&attribute_recv.type,&attribute_recv.length);
						if(message_recv.vrsn=='3')
						{
							if(message_recv.type==JOIN && attribute_recv.type==2)
							{
								if(client_count>max_clients)//If the client limit is reached then sending NAK message
								{
									//send NAK message
									sprintf(buffer_message_send,"Client Limit Exceeded");
									attribute_send.payload=buffer_message_send;
									attribute_send.type=1;
									attribute_send.length=36;//32+4
									message_send.vrsn='3';
									message_send.type=NAK;
									message_send.length=40;
									message_send.attribute=&attribute_send;
									packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,buffer_message_send);
									if(sendall(i, buffer_send, packetsize)==-1)										
									{
										perror("send");
									}	
									client_count=client_count-1;
									close(i); 
									FD_CLR(i, &master); // remove from master set
									break;									
								}
								unpack(buffer+8,"s",buffer_username);
								for(j=4;j<=maxfd;j++)
								{	
									if(strcmp(buffer_username,usernames[j])==0)
									{	
										k=0;//if username is already used then NAK message sent
										sprintf(buffer_message_send,"USERNAME used");
										attribute_send.payload=buffer_message_send;
										attribute_send.type=1;
										attribute_send.length=36;//32+4
										message_send.vrsn='3';
										message_send.type=NAK;
										message_send.length=40; 
										message_send.attribute=&attribute_send;
										packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,buffer_message_send);
										if(sendall(i, buffer_send, packetsize)==-1)
										{
											perror("send");
										}
										client_count=client_count-1;
										close(i); 
										FD_CLR(i, &master); // remove from master set
										break;
									}
								} 
								if (k==1)//Username usable
								{
									sprintf(usernames[i],"%s",buffer_username);
									printf("Client %s connected\n",usernames[i]);
									strcpy (list_name,"Names of clients in the chat room: ");
									for(b=4;b<=maxfd;b++)
									{
										strcat (list_name,usernames[b]);
										strcat (list_name,"|");
									}
									//sending ACK messageupon successfull connection
									sprintf(buffer_message_send,"Number of Clients: %d\n",client_count);
									strcat(buffer_message_send,list_name);
									attribute_send.payload=buffer_message_send;
									attribute_send.type=4;
									attribute_send.length=516;
									message_send.vrsn='3';
									message_send.type=ACK;
									message_send.length=520; 
									message_send.attribute=&attribute_send;
									packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,buffer_message_send);			
									if(sendall(i, buffer_send, packetsize)==-1)										
									{
										perror("send");
									}
									//sending ONLINE message
									attribute_send.payload=usernames[i];
									attribute_send.type=2;
									attribute_send.length=20;
									message_send.vrsn='3';
									message_send.type=ONLINE;
									message_send.length=24; 
									message_send.attribute=&attribute_send;
									packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,attribute_send.payload);
									for(j = 0; j <= maxfd; j++) 
									{
										if (FD_ISSET(j, &master)) 
										{
											if (j != socketfd && j != i) 
											{
												if(sendall(j, buffer_send, packetsize)==-1)											
												{
													perror("send");
												}
											}	
										}
									}
								
								}
							}
							if(message_recv.type==SEND && attribute_recv.type==4)//received chat message
							{ // Forwarding the message back
									unpack(buffer+8, "s", buffer_message_recv);
									sprintf(buffer_message_send, "<%s>:%s",usernames[i],buffer_message_recv);
									attribute_send.payload=buffer_message_send;
									attribute_send.type=4;
									attribute_send.length=516; 
									message_send.vrsn='3';
									message_send.type=FWD;
									message_send.length=520; 
									message_send.attribute=&attribute_send;
									packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,buffer_message_send);
									for(j = 0; j <= maxfd; j++) 
									{
										if (FD_ISSET(j, &master)) 
										{
											if (j != socketfd && j != i) 
											{
												if(sendall(j, buffer_send, packetsize)==-1)											
												{
													perror("send");
												}
											}	
										}
									}
							}
							if(message_recv.type==IDLE)
							{	//sending IDLE message	
								attribute_send.payload=usernames[i];
								attribute_send.type=2;
								attribute_send.length=20;
								message_send.vrsn='3';
								message_send.type=IDLE;
								message_send.length=24;
								message_send.attribute=&attribute_send;
								packetsize=pack(buffer_send,"cchhhs",message_send.vrsn,message_send.type,message_send.length,attribute_send.type,attribute_send.length,attribute_send.payload);
								for(j = 0; j <= maxfd; j++) 
								{
									if (FD_ISSET(j, &master)) 
									{
										if (j != socketfd && j != i) 
										{
											if(sendall(j, buffer_send, packetsize)==-1)												
											{
												perror("send");
											}
										}	
									}
								}
							}
						}
					}
				}
			}	
		}//end of for loop
	}//end of infinite while loop
}//end main

