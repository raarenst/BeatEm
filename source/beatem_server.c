
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "babelsock.h"
#include "babeltime.h"

static int create_server();
static int send_buffer();

static char g_recvbuf[CLIENT_PACKET_SIZE];
static int g_client_list[SERVER_MAX_NR_OF_CLIENTS];
static int g_nr_clients;
static char g_sendbuf[SERVER_PACKET_SIZE];
static int g_nr_packets;

int main(void) {
  int rv;
  int res;
  int added;
  int server_sock;
  int new_sock;
  int select_list[SERVER_MAX_NR_OF_CLIENTS+1]; // One more for server
  long start_time_ms;
  int x = 0;

  g_nr_packets = 0;
    
  /* Setup the server
   */
  server_sock = create_server();
  if (server_sock < 0) {
    printf("*** ERROR: Could not create server socket: %d\n", server_sock);
    return -1;
  }
    
  /* Setup client list
   */
  g_nr_clients = 0;
  for (int idx=0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
    g_client_list[idx] = 0;
  }

  start_time_ms = babelTimeGetCurrentTime();

  /* Main loop
   */
  while(1) {

    /* Setup select list
     */
    select_list[0] = server_sock;
    for (int idx=0; idx < g_nr_clients; idx++) {
      select_list[idx+1] = g_client_list[idx];        
    }

    /* Wait for server, any client or timeout
     */
    res = babelSockSelect(select_list, g_nr_clients+1, SERVER_HEART_BEAT_S*1000);

    /* Time to send, send the buffer before receiving more
     */
    if (babelTimeGetCurrentTime() > (start_time_ms + SERVER_HEART_BEAT_S)) {
      send_buffer();
      g_nr_packets = 0;
      start_time_ms = babelTimeGetCurrentTime();
    }

    /* Handle the select results
     */
    if (res < 0) {

      /* Select error FIXME handle better
       * OBS select error before clock() sending
       */
      printf("*** ERROR: Select: %d\n", res);
      return 1;
    } else if (res == 0) {
	
      /* We have a timeout, do nothing
       */
    } else {

      /* It is a socket that wants something
       */
      for (int idx=0; idx < res; idx++) {
        //printf("-->socket wants something idx:%d  res:%d\n", idx, res);	
        if (select_list[idx] == server_sock) {
	        printf("----> Server adding client\n");
	  
          /* It is the server, a new client is connecting, FIXME Error?
           */
          new_sock = babelSockAccept(server_sock);
          if (new_sock < 0) {
            printf("Could not accept client: %d\n", new_sock);
            babelSockClose(new_sock);
          } else {
            added = 0;
            for (int idy=0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
              if (g_client_list[idy] == 0) {
                g_client_list[idy] = new_sock;
                g_nr_clients++;
	              added = 1;
                break;
              }
            }
            if (added == 0) {
              printf("Max nr of clients reached, refusing connection.\n");
              babelSockClose(new_sock);
            }
	  }
        } else {

          /* It is a client sending a new package or disconnecting
           */
          rv =  babelSockReadAll(select_list[idx], g_recvbuf, CLIENT_PACKET_SIZE);
          if (rv != CLIENT_PACKET_SIZE) {

            /* Client error, close client connection, remove from list
             */
            printf("*** ERROR: Receive from client error: %d\n", rv);
            babelSockClose(select_list[idx]);
            for (int idy=0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
              if (g_client_list[idy] == select_list[idx]) {
                g_client_list[idy] = 0;
                g_nr_clients--;
                break;
              }
            }
          } else {
	      
            /* Add buf to send buffer
             */
            if (g_nr_packets == SERVER_MAX_NR_OF_PACKETS) {
	            printf("buffer full sending before adding new\n");
              /* Send buffer, then add new one
               * FIXME: check client spamming
               */
              send_buffer();
              g_nr_packets = 0;
            }
	          //printf("adding client packet to buffer\n");
            for (int idy=0; idy < CLIENT_PACKET_SIZE; idy++) {
              g_sendbuf[g_nr_packets*CLIENT_PACKET_SIZE+idy] = g_recvbuf[idy];
            }
            g_nr_packets++;	    
	          printf("--> Received package from client, g_nr_packets:%d tot:%d\n", g_nr_packets, x);
            x = x + 1;
          }
        }
      }
    }
  }

  babelSockClose(server_sock);
  babelSockCleanup();
    
  return 0;
}

int send_buffer() {
  int startbuf;
  int res;

  printf("### Sending buffer! g_nr_clients:%d g_nr_packets:%d\n", g_nr_clients, g_nr_packets);
  
  /* Fill remaining buffer with rnd
   */
  srand((unsigned int)babelTimeGetCurrentTime());
  startbuf = g_nr_packets*CLIENT_PACKET_SIZE;
  printf("startbuf: %d PACKET_SIZE:%d SERVER_PACKET_SIZE:%d Buffer:[",
	 startbuf,
	 CLIENT_PACKET_SIZE,
	 SERVER_PACKET_SIZE);
  for (int idx=startbuf; idx < SERVER_PACKET_SIZE; idx++) {
    g_sendbuf[idx] = (char)rand() % 255; // Use rand and hash
  }
  printf("]\n");
  
  /* Send whole buffer to all clients
   */
  for (int idx=0; idx < g_nr_clients; idx++) {
    if (g_client_list[idx] != 0) {
      res = babelSockWriteAll(g_client_list[idx], g_sendbuf, SERVER_PACKET_SIZE);
      if (res != SERVER_PACKET_SIZE) {
      
        /* Client error, close client connection, remove from list
         */
        printf("Client write error: %d\n", res);
        babelSockClose(g_client_list[idx]);
        g_client_list[idx] = 0;
        g_nr_clients--;
      }
    }
  }
  return 0;
}

int create_server() {
  int res;
  int server_sock;

  res = babelSockInit();
  if (res != BABELSOCK_OK) {
    printf("*** ERROR: Could not initialize babelsock: %d\n", res);
    return -1;
  }

  server_sock = babelSock(BABELSOCK_TCP);
  if (server_sock < 0) {
    babelSockCleanup();
    printf("*** ERROR: Could not create server socket: %d\n", server_sock);
    return -1;
  }

  res = babelSockBind(server_sock, "0.0.0.0", SERVER_PORT);
  if (res != BABELSOCK_OK) {
    babelSockClose(server_sock);
    babelSockCleanup();
    printf("*** ERROR: Could not bind se server socket: %d\n", res);
    return -1;
  }

  res = babelSockListen(server_sock, 5);
  if (res != BABELSOCK_OK) {
    babelSockClose(server_sock);
    babelSockCleanup();
    printf("*** ERROR: Could not set server socket to listen: %d\n", res);
    return -1;
  }
    
  return server_sock;
}
