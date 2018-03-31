// Derek Hu
// COEN 146  Lab (63192)
// Lab 2/3 server.c
// 1/25/18
// This file sets up a server on a specified port that will listen for a client to connect.
// Once connected it will read in an output file name and file content to write to that file.

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main (int, char *[]);

int main (int argc, char *argv[])
{
        int i = 0, n;
        int listenfd = 0, connfd = 0, bytesIn = 0;
        struct sockaddr_in serv_addr;
        char buffer[5], outname[1024];
        FILE *output;
        uint16_t portno;

        // set up
        memset (&serv_addr, '0', sizeof (serv_addr));
        memset (buffer, '\0', sizeof (buffer));
        memset (outname, '\0', sizeof (outname));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
        portno = atoi(argv[1]);
        serv_addr.sin_port = htons (portno);

        // create socket, bind, and listen
        listenfd = socket (AF_INET, SOCK_STREAM, 0);
        bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr));
        listen (listenfd, 10);

        // accept and interact
        connfd = accept (listenfd, (struct sockaddr*)NULL, NULL);

        // get output file name
        read (connfd, outname, sizeof (outname));

		// search down the buffer for the end of the file name '\0'
        for (i = 0; i < sizeof (outname); i++)
        {
                if (outname[i] == '\0')
                {
                        strncpy (outname, outname, i);
                        break;
                }
        }

		// open output for writing
        output = fopen(outname, "wb");
        memset (buffer, '\0', sizeof(buffer));

        // read in file content from client until no more content
        while ((bytesIn = read (connfd, buffer, sizeof (buffer))) > 0)
        {
                // if end of file content, write last bytes and quit
                if (bytesIn < sizeof (buffer))
                {
                        fwrite (buffer, 1, bytesIn, output);
                        break;
                }

                fwrite (buffer, 1, sizeof (buffer), output);
                memset (buffer, '\0', sizeof (buffer));
        }

        fclose (output);
        close (connfd);
}