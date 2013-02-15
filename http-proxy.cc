/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include "http-headers.h"
#include "http-request.h"
#include "http-response.h"
using namespace std;

#define DEBUG_HTTP_PROXY 1 

void debug(string msg)
{
	if(DEBUG_HTTP_PROXY)
	{
		cout << msg << endl;
	}
}

void error(string msg)
{
	cout << msg;
	exit(1);
}

//Reads all of the data on a socket, and puts it in a char buffer.
//Returns pointer to char buffer, sets buffSize to the allocated size
//and dataSize to the amount of data. Caller must free the buffer.
char * readResponse(int sockfd, int& buffSize, int& dataSize)
{
	//TODO: Check for overflow problems

	buffSize = 1024;
	dataSize = 0;
	int bytesRead = 0;
	int tempSize = 1024;
	char* temp = new char[tempSize];
	char* buffer = new char[buffSize];
	while((bytesRead = read(sockfd, temp, tempSize)) > 0)
	{
		//Check if buffer is big enough
		if(buffSize < dataSize + bytesRead)
		{
			char* bigBuffer = new char[buffSize * 2];
			memcpy(buffer, bigBuffer, dataSize);
			buffSize *= 2;
			free(buffer);
			buffer = bigBuffer;
		}
		//Add to buffer
		memcpy(buffer + dataSize, temp, bytesRead);
		dataSize += bytesRead;
	}
	free(temp);
	return buffer;
}

void process(int clientSockfd)
{
	//Read from socket
	int buffSize, dataSize;
	char * buffer = readResponse(clientSockfd, buffSize, dataSize);

	// Create HTTP Request object
	HttpRequest req;
	try
	{
		req.ParseRequest(buffer, dataSize);
	}
	catch(ParseException e)
	{
		debug(e.what());
	}
	
	free(buffer);

	//HTTP Request good, send to server
	string host = req.getHost();
	short port = req.getPort();
	
	// Create buffer for HTTPRequest object
	size_t bufLength = req.GetTotalLength();
	buffer = new char[bufLength];
	req.FormatRequest(buffer);

	//Send Request to server
	int servSockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(servSockfd < 0)
		error("Error opening socket to server");


	hostent *server = gethostbyname(host.c_str());
	if(server == NULL)
	{
		error("No Such server");
	}

	sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	bcopy((char *)server->h_addr,
      (char *)&serv_addr.sin_addr.s_addr,
      server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(servSockfd,&serv_addr,sizeof(serv_addr)) < 0)
  		error("ERROR connecting");

  	int bytesWritten = write(servSockfd, buffer, bufLength);
  	if(bytesWritten < 0)
  	{
  		error("Error writing to servSockfd");
  	}
	free(buffer);
	//Listen For response from server
	buffer = readResponse(servSockfd, buffSize, dataSize);

	//Send back to client
	bytesWritten = write(clientSockfd, buffer, dataSize);
	if(bytesWritten < 0)
		error("Error writing to clientSockfd");

	//cout << "Message: " << buffer << endl;
}

int main (int argc, char *argv[])
{
 	// command line parsing
	if(argc > 1)
	{
		cout << "Too many arguments" << endl;
		return 1;
	}	

	int sockfd, newsockfd;
	uint16_t portnum = 14405;
	sockaddr_in servAddr, cliAddr;


	//Create Socket
	debug("Creating Socket");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("Error opening socket");

	//Create servAddr struct
	//Zero out struct
	//bzero((char *) &servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port =  htons(portnum);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	//Bind Socket
	debug("Binding Socket");
	if(bind(sockfd, (sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		error("Error binding socket");

	debug("Listening to Socket");
	if(listen(sockfd, 10) < 0)
		error("Error listening to socket");


	unsigned int cliLength = sizeof(cliAddr);

	while (1)
	{
		debug("Accepting Socket");
		newsockfd = accept(sockfd, (sockaddr *) &cliAddr, &cliLength);
		if(newsockfd < 0)
			error("Error on accept");
		int pid = fork();
		if(pid < 0)
		{
			error("Error on Fork");
		}
		else if (pid == 0) //Child
		{
			close(sockfd);
			process(newsockfd);
			exit(0);
		}
		else //Parent
		{
			close(newsockfd);
		}
	}

	return 0;
}
