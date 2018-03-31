// Derek Hu
// COEN 146  Lab (63192)
// Lab 2/3 client.c
// 1/25/18
// This file creates a client that will attempt to connect to a server at specified port and ip address. 
// Once connected it will send an output file name and content from an input file for the server to write.

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main (int, char *[]);

int main (int argc, char *argv[])
{
        int sockfd = 0, n = 0;
        char fileBuffer[1024], buffer[10];
        FILE *input;
        struct sockaddr_in serv_addr;
        uint16_t portno;

        if (argc != 5)
        {
                printf ("Usage: %s <ip of server> \n",argv[0]);
                return 1;
        }

        // set up
        memset (fileBuffer, '\0', sizeof (fileBuffer));
        memset (buffer, '\0', sizeof (buffer));
        memset (&serv_addr, '0', sizeof (serv_addr));

        // open socket
        if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf ("Error : Could not create socket \n");
                return 1;
        }

        // set address
        portno = atoi(argv[1]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons (portno);

        if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
        {
                printf ("inet_pton error occured\n");
                return 1;
        }

		// open input file for reading
        input = fopen(argv[3], "rb");

        // connect
        if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
        {
                printf ("Error : Connect Failed \n");
                return 1;
        }

        // send output file name
        strncpy (fileBuffer, argv[4], strlen (argv[4]));
        write (sockfd, fileBuffer + '\0', strlen (argv[4]) + 1);
        memset (fileBuffer, '\0', sizeof (fileBuffer));
        memset (buffer, '\0', sizeof (buffer));

        // read in input file and send to server until end
        while ((n = fread(buffer, 1, sizeof (buffer), input)) > 0)
        {
                write (sockfd, buffer, n);
                memset (buffer, '\0', sizeof (buffer));
        }

        fclose (input);
        close (sockfd);

        return 0;
}