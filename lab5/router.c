/*
Derek Hu
Lab 5 router.c
3/16/2018
Simulate routers sending & receiving costs between nodes and updating with link-state algorithm
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <limits.h>

typedef struct {
        char name[50];
        char ip[50];
        int port;
} MACHINE;

int i, j; // variables for read_files()
int in[3], out[3], host1, host2, weight, costs[100][100], k, l; // variables for  receive_info()
int min, m, mindex; // variables for distance()
int wait_time, a, b, src, ct, d; // variables for link_state()
int sock, x; // variables for send_info()
int nhbr, new_cost, y, z; // variables for get_cost()
int id, n, port, this_sock; // variables for main()
FILE *costs_file, *hosts_file;
pthread_mutex_t mutex;
MACHINE hosts[100];
pthread_t receive;
pthread_t link_st;
time_t updated;

void read_files (FILE *costs_file, FILE *hosts_file) { // read initial costs & hosts file and output contents
        for (i = 0; i < n; i++) {
                for (j = 0; j < n; j++) {
                        if (fscanf (costs_file, "%d", &costs[i][j]) != 1)
                                break;
                        printf ("%d ", costs[i][j]);
                }

                printf ("\n");
        }

        for (i = 0; i < n; i++) {
                if (fscanf (hosts_file, "%s %s %d", &hosts[i].name, &hosts[i].ip, &hosts[i].port) < 1)
                        break;
                printf ("%s %s %d\n", hosts[i].name, hosts[i].ip, hosts[i].port);
        }
}

void *receive_info () { // get new costs from other routers and update table, output new table
        while (1) {
                recvfrom (this_sock, &in, sizeof (in), 0, NULL, NULL);

                host1 = ntohl (in[0]);
                host2 = ntohl (in[1]);
                weight = ntohl (in[2]);

                pthread_mutex_lock (&mutex);
                costs[host1][host2] = weight;
                costs[host2][host1] = weight;

                for (k = 0; k < n; k++) {
                        for (l = 0; l < n; l++) {
                                printf ("%d ", costs[k][l]);
                        }

                        printf ("\n");
                }

                printf ("\n");

                pthread_mutex_unlock (&mutex);
        }
}

int distance (int dist[], int visited[]) { // calculate and return min distance
        min = INT_MAX;

        for (m = 0; m < n; m++) {
                if (visited[m] == 0 && dist[m] < min) {
                        min = dist[m];
                        mindex = m;
                }
        }

        return mindex;
}

void *link_state () { // perform Dijkstra's algorithm and output least costs between nodes
        updated = time(NULL);

        while (1) {
                wait_time = (rand()%10 * 2);

                int dist[n], visited[n], temp[n][n];

                if ((time(NULL) - updated) > wait_time) {
                        pthread_mutex_lock (&mutex);

                        for (src = 0; src < n; src++) {
                                for (a = 0; a < n; a++) {
                                        dist[a] = INT_MAX;
                                        visited[a] = 0;
                                }

                                dist[src] = 0;

                                for (ct = 0; ct < n - 1; ct++) {
                                        d = distance(dist, visited);
                                        visited[d] = 1;

                                        for (b = 0; b < n; b++) {
                                                if (visited[b] == 0 && dist[d] != INT_MAX && dist[d] + costs[d][b] < dist[b])
                                                        dist[b] = dist[d] + costs[d][b];
                                        }
                                }

                                for (a = 0; a < n; a++) {
                                        temp[src][a] = dist[a];
                                        temp[a][src] = dist[a];
                                }
                        }
                        printf("New costs:\n");

                        for (a = 0; a < n; a++) { // print least costs
                                for (b = 0; b < n; b++) {
                                        printf("%d ", temp[a][b]);
                                }

                                printf("\n");
                        }

                        printf("\n");
                }

                pthread_mutex_unlock (&mutex);
                updated = time(NULL);
        }
}

void send_info () { // send new costs to other routers using UDP
        struct sockaddr_in destAddr[n];
        socklen_t addr_size[n];

        for (x = 0; x < n; x++) {
                destAddr[x].sin_family = AF_INET;
                destAddr[x].sin_port = htons (hosts[x].port);
                inet_pton (AF_INET, hosts[x].ip, &destAddr[x].sin_addr.s_addr);
                memset (destAddr[x].sin_zero, '\0', sizeof (destAddr[x].sin_zero));
                addr_size[x] = sizeof destAddr[x];
        }

        /*Create UDP socket*/
        sock = socket (PF_INET, SOCK_DGRAM, 0);

        for (x = 0; x < n; x++) {
                if (x != id)
                        sendto (sock, &out, sizeof(out), 0, (struct sockaddr *)&(destAddr[x]), addr_size[x]);
        }
}

void get_cost () { // receive user input with new cost to node
        printf ("Enter new cost from node %d: <neighbor> <cost>\n", id);
        scanf ("%d %d", &nhbr, &new_cost);

        pthread_mutex_lock (&mutex);

        costs[id][nhbr] = new_cost;
        costs[nhbr][id] = new_cost;

        out[0] = htonl (id);
        out[1] = htonl (nhbr);
        out[2] = htonl (new_cost);

        send_info();

        for (y = 0; y < n; y++) {
                for (z = 0; z < n; z++) {
                        printf ("%d ", costs[y][z]);
                }
                printf ("\n");
        }

        pthread_mutex_unlock (&mutex);
}

int main (int argc, char *argv[]) {
        if (argc != 5) {
                printf ("Need id, # machines, costs file, hosts file");
                return 1;
        }

        sscanf (argv[1], "%d", &id);
        sscanf (argv[2], "%d", &n);
        n = 4;

        costs_file = fopen(argv[3], "r");
        hosts_file = fopen(argv[4], "r");
        read_files(costs_file, hosts_file);

        port = hosts[id].port;

        struct  sockaddr_in thisAddr, otherAddr;
        struct sockaddr_storage thisStorage;
        socklen_t thisAddr_size, otherAddr_size;

        // configure address
        thisAddr.sin_family = AF_INET;
        thisAddr.sin_port = htons ((short)port);
        thisAddr.sin_addr.s_addr = htonl (INADDR_ANY);
        memset ((char *)thisAddr.sin_zero, '\0', sizeof (thisAddr.sin_zero));
        thisAddr_size = sizeof thisStorage;

        // create socket
        if ((this_sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                printf ("socket error\n");
                return 1;
        }

        // bind
        if (bind (this_sock, (struct sockaddr *)&thisAddr, sizeof (thisAddr)) != 0)
        {
                printf ("bind error\n");
                return 1;
        }

        pthread_mutex_init(&mutex, NULL);
        pthread_create(&receive, NULL, receive_info, NULL);
        pthread_create(&link_st, NULL, link_state, NULL);

        get_cost();
        sleep(10);
        get_cost();
        sleep(10);
        get_cost();

        return 1;
}
