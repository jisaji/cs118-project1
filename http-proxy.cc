/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>


#include "http-headers.h"
#include "http-request.h"
#include "http-response.h"
using namespace std;

void error(char * msg)
{
	cerr << msg;
	exit(1);
}

int main (int argc, char *argv[])
{
 	// command line parsing
	if(argc > 1)
	{
		cout << "Too many arguments" << endl;
		return 1;
	}	
  	
  	char buffer[1024];
  	int sockfd;
  	uint16_t portnum = 14805;
	struct sockaddr_in servAddr, cliAddr;

	//Create Socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("Error opening socket");

	//Create servAddr struct
	//Zero out struct
	bzero((char *) &servAddr, sizeof(servAddr));
	servAddr.sa_family = AF_INET;
	servAddr.sin_port =  htons(portnum);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	//Bind Socket
	if(bind(sockfd, servAddr, sizeof(servAddr)) < 0)
		error("Error binding socket");

	if(listen(sockfd, 10) < 0)
		error("Error listening to socket");
  	
  	newsockfd = accept(sockfd, &cliAddr, sizeof(cliAddr));
  	if(newsockfd < 0)
  		error("Error on accept");

  	int len = read(newsockfd, buffer, 1024);
  	cout << "Message: " << buffer << endl;
  	return 0;
}
