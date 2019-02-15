# Owner: Srinivas Prahalad Sumukha\
UIN: 627008254\
Functionality: Makefile to complie client and server
CC = gcc
CFLAGS = -c #-Wall
output: client server #make if there is any changes to client or server

A: client.cpp #compiles client.cpp
	$(CC) $(CFLAGS) client.cpp
	#gcc -o client client.cpp

B: server.cpp #complies server.cpp
	$(CC) $(CFLAGS) server.cpp
	#gcc -o server server.cpp 

clean:
	rm client server


