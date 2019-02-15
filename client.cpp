/*********************************************************************************************
Owner: Srinivas Prahalad Sumukha
Uin: 627008254
Questions solved: All situations including the bonus questions
Function: client in SBC Protocol

**********************************************************************************************/



#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define debug 0
#define String_size 512
#define JOIN 2
#define FWD 3
#define SEND 4
#define NAK 5
#define OFFLINE 6
#define ACK 7
#define ONLINE 8
#define IDLE 9


struct sbcp_message
{
	int8_t vrsn;
	int8_t type;
	int16_t length;
	struct sbcp_attribute *payload;
}sbcp_msg,sbcp_msg_rcv;

struct sbcp_attribute
{
	int type;
	int length;
	char *payload_attribute;
}sbcp_attr,sbcp_attr_rcv;

int time_des = 0;
struct timeval time_count;
fd_set timefds;
fd_set readfds;
int status,flag = 0;
char buffer[String_size];
int receive_from_ack;
int receiving_size = sizeof (buffer);
socklen_t *fromlength;
addrinfo server, *server_ptr;
void *address_ptr_void;
char *address, *port, *name;
char username_of_client[16];
char ipstr[INET6_ADDRSTRLEN];
char buff[1000];
char buff_rcv[1024];
char *msg;
int socketfd;
int16_t size_of_packet;
int i_return;
int string_len;
char input_string[String_size];

void packi16(char *buf, unsigned int i)
{
	*buf++ = i>>8; *buf++ = i;
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
				h = (int16_t)va_arg(ap, int); // promoted
				packi16(buf, h);
				buf += 2;
				break;
			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int); // promoted
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
				if (isdigit(*format)) { // track max str len
					maxstrlen = maxstrlen * 10 + (*format-'0');
				}
		}
		if (!isdigit(*format)) maxstrlen = 0;
	}
	va_end(ap);
}

int writen(int socketfd, int flag) {
#if debug
	printf("inside writen\n");
#endif
	//int string_len;
	fgets(input_string,100,stdin);
	string_len = strlen(input_string) -1;
	//msg = strtok(msg, "\n");
	if (input_string[string_len] == '\n')
		input_string[string_len] = '\0';	
	msg = &input_string[0];
#if debug
	printf("Client %s: %s \n",name,msg);
#endif
	return 0;
}
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
int readline(int socketfd, char buffer[], int receiving_size, int flag) {
#if debug
	printf("inside readline");
#endif	
	int receive_from_ack = recv(socketfd, buffer, receiving_size, flag);
#if debug
	printf("after readline");
#endif
	if(receive_from_ack < 0)
	{
#if debug
		printf("receiving error");
#endif
		perror("error while receiving");
	}
	buffer[receive_from_ack] = '\0';	
	unpack(buffer,"cchhhs",&sbcp_msg_rcv.vrsn,&sbcp_msg_rcv.type,&sbcp_msg_rcv.length,&sbcp_attr_rcv.type,&sbcp_attr_rcv.length,buff_rcv);
	//put case statement here
	if(sbcp_msg_rcv.vrsn=='3')
	{
		if(sbcp_msg_rcv.type==FWD)
		{
			printf("FWD MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
		}
		if(sbcp_msg_rcv.type==ONLINE)
		{
			printf("ONLINE MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
		}
		if(sbcp_msg_rcv.type==OFFLINE)
		{
			printf("OFFLINE MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
		}
		if(sbcp_msg_rcv.type==ACK)
		{
			printf("ACK MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
		}
		if(sbcp_msg_rcv.type==NAK)
		{
			printf("NAK MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
			exit(1);
		}
		if(sbcp_msg_rcv.type==IDLE)
		{
			printf("IDLE MESSAGE> ");
			fputs(buff_rcv,stdout);
			printf("\n");
		}
	}




#if debug
	printf("\nClient Received: %s \n",buff_rcv);
	printf("\n at the end\n");
	printf("\nClient Received:");
#endif
	return receive_from_ack;
}

int main(int argc, char *argv[]) {

#if debug
	printf("\nEnter text\n");

#endif
	if(argc != 4)
	{
		printf("enter valid args\n");
	}
	name = argv[1];
	address = argv[2];
	port = argv[3];
	server.ai_family = AF_UNSPEC; //can be AF_INET
	server.ai_socktype = SOCK_STREAM; 
	server.ai_flags = AI_PASSIVE;
	status = getaddrinfo(address, port, &server, &server_ptr);
	if(status !=0) {
#if debug
		printf("\nunable to connect to ip address %s\n",address);
#endif
		exit(0);
	}
	addrinfo *ptr_copy;
	ptr_copy = server_ptr;
	for(ptr_copy = server_ptr; ptr_copy != NULL; ptr_copy= ptr_copy->ai_next) {
		struct sockaddr_in *ptr_with_address = (struct sockaddr_in *) ptr_copy->ai_addr;
		address_ptr_void = &(ptr_with_address->sin_addr);
		socketfd = socket(server_ptr->ai_family, server_ptr->ai_socktype, server_ptr->ai_protocol);
		if (socketfd == -1) {
#if debug
			printf("\nError at the socket call\n");
#endif
			perror("Error at socket");
			continue;
		}

		//printf("before connect ack");
		int connect_ack = connect(socketfd,server_ptr->ai_addr, server_ptr->ai_addrlen); //returns negitive value on failure to connect
		//printf("after connect ack");
		if(connect_ack == -1) {
#if debug
			printf("\ncould not connect\n");
#endif
			perror("Problem with connect");
			//exit(0);
		}
		break;
	}
	//printf("before init");
	strcpy(username_of_client , name);
	sbcp_attr.payload_attribute = username_of_client;
	sbcp_attr.type = 2;
	sbcp_attr.length = 20;
	sbcp_msg.vrsn = '3';
	sbcp_msg.type = 2;
	sbcp_msg.length = 24;
	sbcp_msg.payload = &sbcp_attr;
	size_of_packet = pack(buff, "cchhhs", sbcp_msg.vrsn, sbcp_msg.type, sbcp_msg.length, sbcp_attr.type, sbcp_attr.length, sbcp_attr.payload_attribute);
	//printf("after init");
	if(sendall(socketfd, buff, size_of_packet)==-1)
		//if(send(socketfd, buff, size_of_packet, flag) < 0)
	{
		printf("error sending username to client\n");
		perror("send usename error");
		exit(0);
	}
	//writen(socketfd, flag);
	sbcp_attr.payload_attribute = input_string;
	sbcp_attr.type = 4;
	sbcp_attr.length = 516;
	sbcp_msg.vrsn = '3';
	sbcp_msg.type = 4;
	sbcp_msg.length = 520;
	sbcp_msg.payload = &sbcp_attr;
	int STDIN = 0;
	FD_SET(STDIN , &readfds);
	FD_SET(socketfd , &readfds);
#if debug
	printf("socketfd %d",socketfd);
#endif

	time_count.tv_sec = 10;
	time_count.tv_usec = 0;
	int idle = 0;

	while(1)
	{
		int select_value;
		select_value = select(socketfd+1, &readfds, NULL,NULL, &time_count);
		if(select_value < 0)
		{
			printf("error in select\n");
			exit(0);
		}
		if(!(FD_ISSET(STDIN,&readfds)) && time_count.tv_sec == 0 && idle == 0){
			//strcpy(sbcp_attr.payload_attribute,"\0");
			sbcp_attr.type = NULL;
			sbcp_attr.length = 0;
			sbcp_msg.vrsn = '3';
			sbcp_msg.type = 9;
			sbcp_msg.length = 4;
			sbcp_msg.payload = &sbcp_attr;
			size_of_packet = pack(buff, "cchhh", sbcp_msg.vrsn, sbcp_msg.type, sbcp_msg.length, sbcp_attr.type, sbcp_attr.length, input_string);
			//packet = sbcp_to_string(VERSION,IDLE,0,NULL);
			int sendto_ack = send(socketfd, buff, size_of_packet, flag); //checking for any error while sending
			if(sendto_ack <= -1)
			{
				printf("\nerror sending\n");
				exit(0);
			}
			time_count.tv_sec = 10;
			printf("You are idle\n");
			idle = 1;
			FD_SET(STDIN , &readfds);
			FD_SET(socketfd , &readfds);
		};
		for(i_return = 0 ; i_return <= socketfd ; i_return++)
		{
			if(FD_ISSET(i_return,&readfds))//if(FD_ISSET(socketfd,&readfds))
			{
				idle = 0;
#if debug
				//strtok(msg, "\n");
#endif

#if debug
				printf("i1 %d\n",i_return);
#endif
				if(i_return == 0)	
				{
#if debug
					printf("before writen\n");
#endif
					time_count.tv_sec = 10;
					idle = 0;
					writen(socketfd, flag); // function call to write
					size_of_packet = pack(buff, "cchhhs", sbcp_msg.vrsn, sbcp_msg.type, sbcp_msg.length, sbcp_attr.type, sbcp_attr.length, input_string);
#if debug
					printf("msg after pack %s\n",msg);
#endif
					sbcp_attr.payload_attribute = input_string;
					sbcp_attr.type = 4;
					sbcp_attr.length = 516;
					sbcp_msg.vrsn = '3';
					sbcp_msg.type = 4;
					sbcp_msg.length = 520;
					sbcp_msg.payload = &sbcp_attr;
					int sendto_ack = sendall(socketfd, buff, size_of_packet); //checking for any error while sending
					if(sendto_ack <= -1) 
					{
						printf("\nerror sending\n");
						exit(0);
					}
					//time_count.tv_sec = 10;
					//idle = 0;
#if debug
					printf("i_return %d and socketfd %d \n",i_return,socketfd);
#endif
				}

#if debug				
				printf("i1 %d\n",i_return);							
#endif
				if(i_return == socketfd)
				{
					int receiving_data = readline(socketfd,buffer,receiving_size, flag); // function call to read
					if(receiving_data == -1) {
						printf("\nerror receiving\n");
					}
				}//printf("I am going out of FD_ISSET\n");
			}FD_SET(0, &readfds);
			FD_SET(socketfd, &readfds);
#if debug
			printf("socketfd %d i_return %d\n",socketfd, i_return);
#endif 
		}time_count.tv_sec = 10;
	}
	close(socketfd);
	freeaddrinfo(server_ptr);
	return 0;
}


