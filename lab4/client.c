/*//##############
Derek Hu
COEN 146 Lab (63192)
Lab 4 client.c
2/23/18
This is the client that follows stop-and wait and rdt2.2. It sends a file to a server in packets, waiting for ACKs before sending the next packet
This version also includes a 2 second timeout if it does not receive any ACKs after sending a packet
*///##############

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main (int argc, char *argv[]) {
        int sock, portNum, nBytes, ack_size, xor, chksm, chk_save, ackchk, n, rv, end_count = 0;
        char buffer[10];
        FILE *input;
        struct sockaddr_in serverAddr;
        socklen_t addr_size;

        if (argc != 5) {
                printf ("Need the port number, IP address, input name, output name.\n");
                return 1;
        }

        typedef struct {
                int seq_ack;
                int length;
                int checksum;
        } HEADER;

        typedef struct {
                HEADER header;
                char data[10];
        } PACKET;

        struct timeval tv;
        fd_set readfds;
        FD_ZERO (&readfds);
        FD_SET (sock, &readfds);
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        int checksum (PACKET packet) { // Checksum function that takes a packet and returns its 1 byte checksum
                xor = (packet.header.seq_ack >> (8*0)) & 0xff;
                for (n = 1; n < 4; n++)
                        xor ^= (packet.header.seq_ack >> (8*n)) & 0xff;
                for (n = 0; n < 4; n++)
                        xor ^= (packet.header.length >> (8*n)) & 0xff;
                for (n = 0; n < 4; n++)
                        xor ^= (packet.header.checksum >> (8*n)) & 0xff;;
                for (n = 0; n < strlen (packet.data); n++) {
                        xor ^= packet.data[n];
                }
                return xor;
        }

        PACKET packet_send; // packet for sending message
        PACKET packet_recv; // packet for receiving ACKs

        PACKET my_send (PACKET packet_send) {
                while (1) {
                        if (end_count > 2 && packet_send.header.length == 0) { // If count is 3 or more and length is 0 close and end program
                                break;
                        }
                        packet_send.header.checksum = 0;
                        chksm = checksum (packet_send); // Calculate checksum of message

                        if ((rand() % 5) < 4 || packet_send.header.length == 0) { // Randomize whether to resend a good or bad checksum
                                printf ("Good checksum\n");
                                packet_send.header.checksum = chksm;
                        } else {
                                printf("Bad checksum\n");
                                packet_send.header.checksum = 0;
                        }
                        if ((rand() % 6) < 4) { // Randomize whether to resend message or not
                                printf ("Sending\n");
                                sendto (sock, &packet_send, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);
                        } else {
                                printf ("Not sending\n");
                        }

                        FD_ZERO (&readfds);
                        FD_SET (sock, &readfds);
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;
                        rv = select (sock + 1, &readfds, NULL, NULL, &tv); // Check if data is received

                        if (rv == 0) {
                                // Timeout, no data received so resend
                        } else if (rv == 1) {
                                ack_size = recvfrom (sock, &packet_recv, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);
                                chk_save = packet_recv.header.checksum; // Save and calculate ACK checksums to compare
                                packet_recv.header.checksum = 0;
                                ackchk = checksum (packet_recv);

                                if (ack_size > 0 && packet_recv.header.seq_ack == packet_send.header.seq_ack && chk_save == ackchk) { // Check if ACK = SEQ & checksums match. If it does, ((SEQ + 1) % 2) and continue to next message
                                        printf ("ACK%i accepted\n\n", packet_send.header.seq_ack);
                                        packet_send.header.seq_ack = ((packet_send.header.seq_ack + 1) % 2); // go to next seq/ack state

                                        memset (buffer, '\0', sizeof (buffer));
                                        memset (packet_send.data, '\0', sizeof (packet_send.data));
                                        return packet_send;
                                } else { // No matching ACK found so resend message and loop back into waiting for ACK
                                        printf ("ACK rejected, resending\n");
                                }
                        }
                        end_count++; // Increment count to close after 3
                }
        }

        // configure address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons (atoi (argv[1]));
        inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
        memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
        addr_size = sizeof serverAddr;

        /*Create UDP socket*/
        sock = socket (PF_INET, SOCK_DGRAM, 0);

        packet_send.header.seq_ack = 0;
        input = fopen(argv[3], "rb"); // open input file for reading
        srand(time(0));

        memset (buffer, '\0', sizeof (buffer));
        memset (packet_send.data, '\0', sizeof (packet_send.data));

        // Sending input data to server
        strcpy (packet_send.data, argv[4]);
        packet_send.header.length = strlen (packet_send.data);
        packet_send = my_send (packet_send);

        // Send packet to server
        while (1) {//(nBytes = fread(buffer, 1, sizeof (buffer), input)) > 0) {
                if ((nBytes = fread(buffer, 1, sizeof (buffer), input)) <= 0) { // if no more data to send
                        memset (packet_send.data, '\0', sizeof (packet_send.data));
                        packet_send.header.length = strlen (packet_send.data); // Send packet with no data to notify server file is complete
                        end_count = 0;
                        packet_send = my_send (packet_send);
                        break;
                }

                strcpy (packet_send.data, buffer);
                printf ("Sending packet%i to server with msg %s\n", packet_send.header.seq_ack, packet_send.data); // Send message to server
                packet_send = my_send (packet_send);
        }

        fclose (input);
        close (sock);

        return 0;
}
