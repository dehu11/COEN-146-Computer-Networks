#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
        int sock, nBytes, xor, chksm, chk_save, chk_last, msg_size, n, turn = 0;
        char buffer[10];
        FILE *output;
        struct sockaddr_in serverAddr, clientAddr;
        struct sockaddr_storage serverStorage;
        socklen_t addr_size, client_addr_size;

        if (argc != 2) {
                printf ("need the port number\n");
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

        // init
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons ((short)atoi (argv[1]));
        serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
        memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
        addr_size = sizeof (serverStorage);

        // create socket
        if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                printf ("socket error\n");
                return 1;
        }

        // bind
        if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0) {
                printf ("bind error\n");
                return 1;
        }

        PACKET packet_ack; // packet for sending ACKs
        PACKET packet_msg; // packet for receiving messages
        packet_ack.header.seq_ack = 0;
        memset (packet_ack.data, '\0', sizeof (packet_ack.data));
        packet_ack.header.length = 0;

        srand(time(0));

        // Receiving output name from client
        while ((msg_size = recvfrom (sock, &packet_msg, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size)) > 0) {
                chk_save = packet_msg.header.checksum; // Save and calculate checksums to compare
                packet_msg.header.checksum = 0;
                chksm = checksum (packet_msg);
                printf ("received checksum %i vs calculated %i\n", chk_save, chksm);

                if (packet_msg.header.length == 0) { // If no more data received file is complete, so break
                        packet_ack.header.checksum = 0;
                        chksm = checksum (packet_ack); // Calculate checksum of ACK
                        packet_ack.header.checksum = chksm;
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        break;
                } else if (msg_size > 0 && packet_msg.header.seq_ack == packet_ack.header.seq_ack && chksm == chk_save) { //Check if ACK = SEQ & checksums match. If it does, open file and wait for data to write
                        printf ("msg received, sending ACK0\n\n");
                        chk_last = chk_save;
                        packet_ack.header.checksum = 0;
                        chksm = checksum (packet_ack); // Calculate checksum of ACK
                        if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                                packet_ack.header.checksum = chksm;
                        } else {
                                packet_ack.header.checksum = -1;
                        }

                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        if (turn == 0) {
							output = fopen(packet_msg.data, "wb"); // open output file for writing
							turn++;
						} else {
							strncpy (buffer, packet_msg.data, strlen (packet_msg.data));
							fwrite (buffer, 1, strlen (packet_msg.data), output); // Write data to file
						}
                        packet_ack.header.seq_ack = 1;
                } else if (msg_size > 0 && chk_last == chk_save) {
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                        packet_ack.header.checksum = 0;
                        if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                                packet_ack.header.checksum = chksm;
                        } else {
                                packet_ack.header.checksum = 0;
                        }
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                } else {
                        printf ("bad msg rejected\n"); // SEQ != ACK or wrong checksum, so send a NAK
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                }
        }

        /*memset (buffer, '\0', sizeof (buffer));
        printf("here");
        // Receiving input data from client
        while (1) {
                msg_size = recvfrom (sock, &packet_msg, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);
                chk_save = packet_msg.header.checksum; // Save and calculate checksums to compare
                packet_msg.header.checksum = 0;
                chksm = checksum (packet_msg);
                printf ("received checksum %i vs calculated %i\n", chk_save, chksm);

                if (packet_msg.header.length == 0) { // If no more data received file is complete, so break
                        packet_ack.header.checksum = 0;
                        chksm = checksum (packet_ack); // Calculate checksum of ACK
                        packet_ack.header.checksum = chksm;
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        break;
                } else if  (msg_size > 0 && chk_last == chk_save) {
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                        packet_ack.header.checksum = 0;
                        if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                                packet_ack.header.checksum = chksm;
                        } else {
                                packet_ack.header.checksum = -1;
                        }
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                } else if (msg_size > 0 && packet_msg.header.seq_ack == packet_ack.header.seq_ack && chksm == chk_save) { // Check if ACK = SEQ & checksums match. If it does, write data to file and send good ACK
                        printf ("msg accepted, sending ACK%i\n\n", packet_ack.header.seq_ack);
                        strncpy (buffer, packet_msg.data, strlen (packet_msg.data));
                        fwrite (buffer, 1, strlen (packet_msg.data), output); // Write data to file

                        chk_last = chk_save;
                        packet_ack.header.checksum = 0;
                        chksm = checksum (packet_ack); // Calculate checksum of ACK
                        if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                                packet_ack.header.checksum = chksm;
                        } else {
                                packet_ack.header.checksum = -1;
                        }
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                } else {
                        printf ("bad msg rejected\n"); // SEQ != ACK or wrong checksum, so send a NAK
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                        sendto (sock, &packet_ack, sizeof (PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
                        packet_ack.header.seq_ack = ((packet_ack.header.seq_ack + 1) % 2);
                }

                memset (buffer, '\0', sizeof (buffer));
        }*/

        fclose (output);
        close (sock);

        return 0;
}
