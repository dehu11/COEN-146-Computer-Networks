#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main (int argc, char *argv[]) {
        int sock, portNum, nBytes, ack_size, resend = 0, xor, chksm, chk_save, ackchk, n;
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

        // configure address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons (atoi (argv[1]));
        inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
        memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
        addr_size = sizeof serverAddr;

        /*Create UDP socket*/
        sock = socket (PF_INET, SOCK_DGRAM, 0);

        PACKET packet_send; // packet for sending message
        PACKET packet_recv; // packet for receiving ACKs
        packet_send.header.seq_ack = 0;

        input = fopen(argv[3], "rb"); // open input file for reading

        srand(time(0));

        /*// Sending output name to server
        while (1) { // Continue in looping wait-state until an ACK matching SEQ is received
                strcpy (packet_send.data, argv[4]);
                packet_send.header.length = strlen (packet_send.data);
                packet_send.header.checksum = 0;
                chksm = checksum (packet_send); // Calculate checksum of messgage

                if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                        packet_send.header.checksum = chksm;
                } else {
                        packet_send.header.checksum = 0;
                }

                // Send packet with output name to server
                printf ("Sending packet%i to server with msg %s\n", packet_send.header.seq_ack, packet_send.data);
                sendto (sock, &packet_send, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);

                ack_size = recvfrom (sock, &packet_recv, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);
                chk_save = packet_recv.header.checksum; // Save and calculate ACK checksums to compare
                packet_recv.header.checksum = 0;
                ackchk = checksum (packet_recv);
                printf ("received checksum %i vs calculated %i\n", chk_save, ackchk);

                if (ack_size > 0 && packet_recv.header.seq_ack == packet_send.header.seq_ack && chk_save == ackchk) { // Check if ACK = SEQ & checksums match. If it does, ((SEQ + 1) % 2) and continue to next message
                        printf ("ACK%i accepted\n\n", packet_send.header.seq_ack);
                        packet_send.header.seq_ack = ((packet_send.header.seq_ack + 1) % 2);
                        break;
                } else { // No matching ACK found so resend message and loop back into waiting for ACK
                        printf ("ACK rejected, resending\n");
                }
        }*/

        memset (buffer, '\0', sizeof (buffer));
        memset (packet_send.data, '\0', sizeof (packet_send.data));

        // Sending input data to server
        while (1) {
                if (resend != 1) { // If not resending, put next piece of message into packet
					if (strlen (argv[4]) > 0) {
						strcpy (packet_send.data, argv[4]);
						packet_send.header.length = strlen (packet_send.data);
						packet_send.header.checksum = 0;
						chksm = checksum (packet_send); // Calculate checksum of messgage
						memset (argv[4], '\0', sizeof (argv[4]));
					} else {
						if ((nBytes = fread(buffer, 1, sizeof (buffer), input)) <= 0) {
								memset (packet_send.data, '\0', sizeof (packet_send.data));
								packet_send.header.length = strlen (packet_send.data); // Send packet with no data to notify server file is complete
								sendto (sock, &packet_send, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);
								ack_size = recvfrom (sock, &packet_recv, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);
								chk_save = packet_recv.header.checksum; // Save and calculate ACK checksums to compare
								packet_recv.header.checksum = 0;
								ackchk = checksum (packet_recv);
								printf ("received checksum %i vs calculated %i\n", chk_save, ackchk);
								if (ack_size > 0 && packet_recv.header.seq_ack == packet_send.header.seq_ack && chk_save == ackchk) { // Check if ACK = SEQ & checksums match. If it does, ((SEQ + 1) % 2) and continue to next message
										break; // Break if there is no more data to send
								}
						}
						strncpy (packet_send.data, buffer, nBytes);
						packet_send.header.length = strlen (packet_send.data);
						packet_send.header.checksum = 0;
						chksm = checksum (packet_send); // Calculate checksum of message
					}
						
					if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
							packet_send.header.checksum = chksm;
					} else {
							packet_send.header.checksum = 0;
					}
                } else {
                        if ((rand() % 2) == 0) { // Randomize whether to send a good or bad checksum
                                packet_send.header.checksum = chksm;
                        } else {
                                packet_send.header.checksum = 0;
                        }
                        resend = 0;
                }

                // Send packet to server
                printf ("Sending packet%i to server with msg %s\n", packet_send.header.seq_ack, packet_send.data);
                sendto (sock, &packet_send, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);

                ack_size = recvfrom (sock, &packet_recv, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);
                chk_save = packet_recv.header.checksum; // Save and calculate ACK checksums to compare
                packet_recv.header.checksum = 0;
                ackchk = checksum (packet_recv);
                printf ("received checksum %i vs calculated %i\n", chk_save, ackchk);

                if (ack_size > 0 && packet_recv.header.seq_ack == packet_send.header.seq_ack && chk_save == ackchk) { // Check if ACK = SEQ & checksums match. If it does, ((SEQ + 1) % 2) and continue to next message
                        printf ("ACK%i accepted\n\n", packet_send.header.seq_ack);
                        packet_send.header.seq_ack = ((packet_send.header.seq_ack + 1) % 2);

                        memset (buffer, '\0', sizeof (buffer));
                        memset (packet_send.data, '\0', sizeof (packet_send.data));
                } else { // No matching ACK found so resend message and loop back into waiting for ACK
                        printf ("ACK rejected, resending\n");
                        resend = 1;
                }
        }

        memset (packet_send.data, '\0', sizeof (packet_send.data));
        packet_send.header.length = strlen (packet_send.data); // Send packet with no data to notify server file is complete
        sendto (sock, &packet_send, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);

        fclose (input);
        close (sock);

        return 0;
}