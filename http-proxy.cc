/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctime>
#include <errno.h>


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
	//debug("In readResponse()");

	//TODO: Check for overflow problems

	int largeNum = 1024;

	dataSize = 0;
	buffSize = largeNum;
	int bytesRead = 0;
	int tempSize = largeNum;
	char* temp = new char[tempSize];
	char* buffer = new char[buffSize];
	bzero(temp, tempSize);
	bzero(buffer, buffSize);
	
	while((bytesRead = recv(sockfd, temp, tempSize, MSG_DONTWAIT)) > 0)
	{
		debug("BytesRead loop");

		bytesRead = recv(sockfd, temp, tempSize, MSG_DONTWAIT);

		if (bytesRead < 0)
			cout << "ERRNO: " << errno << endl;

		cout << "*** bytesRead: " << temp << "***" << endl;

		//Check if buffer is big enough
		if(buffSize < dataSize + bytesRead)
		{
			debug("*** resizing buffer ***");
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
	debug("End of readResponse()");
	free(temp);
	return buffer;
}

void process(int clientSockfd)
{
	/*time_t startTime, endTime;
	time(&startTime);
	double timeElapsed = 0;*/
	bool persistentConnection = true;

	while(persistentConnection)
	{
		debug("In process loop");

		// Timer
		/*time(&endTime);
		timeElapsed = difftime(endTime, startTime);
		cout << "timeElapsed: " << timeElapsed << endl;
		if (timeElapsed > 5.0)
			break;*/

		//Read from socket
		int buffSize = 0; 
		int dataSize = 0;
		char * buffer = readResponse(clientSockfd, buffSize, dataSize);

		//cout << "Buffer w/ request from client: " << buffer << endl;

		//debug("After readResponse()");

		// Create HTTP Request, Headers objects
		HttpRequest req;
		//HttpHeaders hdrs;
		try
		{
			req.ParseRequest(buffer, dataSize);
			//req.ParseHeaders(buffer, dataSize);
		}
		catch(ParseException e)
		{
			cout << "In catch block!!!" << endl;
			debug(e.what());
		}
		
		//debug("After try-catch");

		free(buffer);

		// Check for persistent connection
		string connHeader = req.FindHeader("Connection");
		cout << "--- conn value: " << connHeader << endl;
		if (connHeader == "close")
		{
			persistentConnection = false;
			debug("close connection specified");
			req.RemoveHeader("Connection");
		}


		// HTTP Request good, send to server
		string path = req.GetPath();
		string host = req.GetHost();
		unsigned short port = req.GetPort();
		
		//cout << "Path: " << path << endl;
		//cout << "Host: " << host << endl;
		//cout << "Port: " << port << endl;

		// Create buffer for HTTPRequest object
		size_t bufLength = req.GetTotalLength();
		buffer = new char[bufLength];
		req.FormatRequest(buffer);

		// Send Request to server
		addrinfo hints;
		addrinfo * result, * rp;
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;

		//Convert unsigned short port to c string
		char portstr[10];
		sprintf(portstr, "%u", port);

		int s = getaddrinfo(host.c_str(), portstr, &hints, &result);

		if(s != 0)
		{
			error("No Such server");
		}
		int servSockfd;	
		for (rp = result; rp != NULL; rp = rp->ai_next) {
	        servSockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	        if (servSockfd == -1)
	            continue;

	    	if (connect(servSockfd, rp->ai_addr, rp->ai_addrlen) != -1)
	            break;                  /* Success */

	       close(servSockfd);
	    }

	    if (rp == NULL) {               /* No address succeeded */
	        error("Could not connect");
	    }

	    free(result);
		debug("Connected to server. Attempting to write to server socket.");

		cout << "*** buff to server: " << buffer << "***" << endl;
		cout << "servSockfd: " << servSockfd << endl;
		cout << "bufLength: " << bufLength << endl;

	  	int bytesWritten = send(servSockfd, buffer, bufLength, 0);
	  	if(bytesWritten < 0)
	  	{
	  		error("Error writing to servSockfd");
	  	}

		//debug("bytesWritten to server. Before free now.");	

		free(buffer);
		sleep(2);
		// Listening to response from server
		debug("Listening for response from server");
		buffer = readResponse(servSockfd, buffSize, dataSize);

		cout << "Buffer w/ response for client: " << buffer << endl;

		//Send back to client
		debug("Sending response to client");
		bytesWritten = send(clientSockfd, buffer, dataSize, 0);
		if(bytesWritten < 0)
			error("Error writing to clientSockfd");


		free(buffer);
		debug("End of request");
		sleep(5);
	}

	debug("Closing connection");
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
	uint16_t portnum = 14459;
	sockaddr_in servAddr, cliAddr;


	//Create Socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("Error opening socket");

	//Create servAddr struct
	//Zero out struct
	//bzero((char *) &servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(portnum);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	//Bind Socket
	if(bind(sockfd, (sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		error("Error binding socket");

	if(listen(sockfd, 10) < 0)
		error("Error listening to socket");


	unsigned int cliLength = sizeof(cliAddr);

	while (1)
	{
		//debug("Top of while loop");
		newsockfd = accept(sockfd, (sockaddr *) &cliAddr, &cliLength);
		debug("Accepted Socket");
		if(newsockfd < 0)
			error("Error on accept");
		//debug("Forking");
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
