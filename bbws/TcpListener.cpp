#include "TcpListener.h"
#include <iostream>
#include <string>
#include <sstream>

int set_non_blocking(int sockfd)
{
	int flags, s;
	flags = fcntl(sockfd, F_GETFL, 0);
	if(flags == -1)
	{
		perror("fcntl");
		return -1;
	}
	flags |= O_NONBLOCK;
	s = fcntl(sockfd, F_SETFL, flags);
	if(s == -1)
	{
		perror("fcntl");
		return -1;
	}
	return 0;
}


int TcpListener::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == -1)
	{
		std::cerr << "socket() failed" << std::endl;
        return 0;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_port);
	inet_pton(AF_INET, m_ipAddress, &hint.sin_addr);

	set_non_blocking(m_socket);
	if (bind(m_socket, (sockaddr*)&hint, sizeof(hint)) == -1)
	{
		std::cerr << "socket() failed" << std::endl;
        return 0;
	}

	// Tell Winsock the socket is for listening 
	if (listen(m_socket, SOMAXCONN) == -1)
	{
		std::cerr << "socket() failed" << std::endl;
        return 0;
	}

	if ((m_epfd = epoll_create1(0)) < 0)
		std::cerr << "epoll_create1() failed" << std::endl;
	// Create the master file descriptor set and zero it
	//FD_ZERO(&m_master);

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	//FD_SET(m_socket, &m_master);
	struct epoll_event event;
	
	event.data.fd = m_socket;
	event.events = EPOLLIN | EPOLLOUT;

	if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_socket, &event))
		std::cerr << "epoll_ctl() failed to add master listen" << std::endl;

	return 0;
}

int TcpListener::run()
{
	// this will be changed by the \quit command (see below, bonus not in video!)
	bool running = true;
	cout << "server start running" << endl;
	struct epoll_event *events = (struct epoll_event *)malloc(sizeof (struct epoll_event) * m_max_events);
	while (running)
	{
		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages. 

		// E.g. You have a server and it's master file descriptor set contains 5 items;
		// the listening socket and four clients. When you pass this set into select(), 
		// only the sockets that are interacting with the server are returned. Let's say
		// only one client is sending a message at that time. The contents of 'copy' will
		// be one socket. You will have LOST all the other sockets.


		// See who's talking to us
		int socketCount = epoll_wait(m_epfd, events, m_max_events, -1);
		
		if (socketCount < 0) {
            std::cerr << "Error calling epoll" << std::endl;
          //  return (EXIT_FAILURE);
        }


		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			int sock = events[i].data.fd;

			// Is it an inbound communication?
			if (sock == m_socket)
			{
				// Accept a new connection
				int client = accept(m_socket, nullptr, nullptr);
				//set_non_blocking(client);
				// Add the new connection to the list of connected clients
				struct epoll_event event;
	
				event.data.fd = client;
				event.events = EPOLLIN | EPOLLOUT;

				if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, client, &event)) {
					std::cerr << "epoll_ctl() failed to insert client fd:" << client << endl;
				}

				onClientConnected(client);
			}
			else // It's an inbound message
			{
				char buf[4096] = {};

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					onClientDisconnected(sock);
					struct epoll_event event;
					if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, sock, &event))
						std::cerr << "epoll_ctl() failed to remove client fd:" << sock << endl;
					
					close(sock);
				}
				else
				{
					onMessageReceived(sock, buf, bytesIn);
				}
			}
		}
	
	}

	// Remove it from the master file list and close the socket
	free(events);
	close(m_socket);

	cout << "server end running" << endl;
	return 0;
}


void TcpListener::sendToClient(int clientSocket, const char* msg, int length)
{
	send(clientSocket, msg, length, 0);
}

void TcpListener::broadcastToClients(int sendingClient, const char* msg, int length)
{
	for (auto con_sock_it = connections.begin(); 
				 con_sock_it != connections.end();
				 con_sock_it++)
	{
		
		if (*con_sock_it != m_socket && *con_sock_it != sendingClient)
		{
			sendToClient(*con_sock_it, msg, length);
		}
	}
}

void TcpListener::onClientConnected(int clientSocket)
{

}

void TcpListener::onClientDisconnected(int clientSocket)
{

}

void TcpListener::onMessageReceived(int clientSocket, const char* msg, int length)
{

}
