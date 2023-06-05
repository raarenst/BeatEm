/*

Client
======
#define CLIENT_PACKET_SIZE 128 //bytes, 64 chars (41 for text)
#define CLIENT_SEND_DELAY 4000
#define CLIENT_HEADER_SIZE 23

Packet: 128 bytes encoded -> 64 characters
Max clients: 16

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "babelsock.h"
#include "babelthread.h"
#include "babeltime.h"
#include "crypto.h"
#include "config.h"

/* An encrypted character is unit16_t thus two bytes.
 */
#define MESSAGE_LEN (CLIENT_PACKET_SIZE/2)
#define TEXT_LEN (MESSAGE_LEN - CLIENT_HEADER_SIZE)

int client_sock;
char send_buf_flag;
char error_flag;
char g_text_buffer[MESSAGE_LEN];
uint16_t g_my_private_key_d = 8419; // 4607
uint16_t g_my_public_key_e = 13219; // 1919
uint16_t g_my_key_n = 32111;        // 5141
uint16_t g_remote_public_key;
uint16_t g_remote_n;
char g_my_key_str[CLIENT_HEADER_SIZE];
char g_remote_key_str[CLIENT_HEADER_SIZE];
char g_server_url[128];

void get_key_str(char *buf, uint16_t public_key, uint16_t key_n) {

  /* 23 chars total
   */
  sprintf(buf, "$%10d:%10d$", public_key, key_n);
}

void print_welcome() {
    printf("==========================================\n");
    printf("           BRUNO CHAT CLIENT\n");
    printf("          For your eyes only!\n");
    printf("==========================================\n");
    printf("My private key (d): %d\n", g_my_private_key_d);
    printf("My public key (e):  %d\n", g_my_public_key_e);
    printf("My n:               %d\n", g_my_key_n);
    printf("Remote public key:  %d\n", g_remote_public_key);
    printf("Remote n:           %d\n", g_remote_n);
    printf("------------------------------------------\n");
    printf("Commands:\n");
    printf("  &genkeys to regenerate keys.\n");
    printf("------------------------------------------\n");	
}

void gen_keys() {
  rsa_key_gen(&g_my_public_key_e, &g_my_private_key_d, &g_my_key_n);
  get_key_str(g_my_key_str, g_my_public_key_e, g_my_key_n);
  print_welcome();
}

void *send_thread_func(void *arg) {

  char send_buffer[CLIENT_PACKET_SIZE]; // Encrypted
  uint8_t header_text[MESSAGE_LEN];
  int res;
  uint16_t *p_sb;
  int x = 0;
  
  for(;;) {
    if (error_flag == 0) {
      if (send_buf_flag == 1) {

	/* Put together header and message
	 */
	strncpy((char*)header_text, g_remote_key_str, CLIENT_HEADER_SIZE);
        strncpy((char*)(header_text+CLIENT_HEADER_SIZE), g_text_buffer, TEXT_LEN);
    	  //printf("\n===========> [%s]\n\n", (char*)header_text);
	
        /* Encode message
	       */
        p_sb = (uint16_t*)send_buffer;
        rsa_encrypt(header_text, p_sb, MESSAGE_LEN, g_remote_public_key, g_remote_n);
	
        /* Send buffer ready to be sent
         */
        res = babelSockWriteAll(client_sock, send_buffer, CLIENT_PACKET_SIZE);
        if (res != CLIENT_PACKET_SIZE) {
          printf("Client write error: %d\n", res);
	        error_flag = 1;
        }
        send_buf_flag = 0;
      } else {

        /* Generate random buffer
         */
        srand(babelTimeGetCurrentTime());
        for (int idx=0; idx < CLIENT_PACKET_SIZE; idx++) {
          send_buffer[idx] = (char)rand() % 255;
        }
        res = babelSockWriteAll(client_sock, send_buffer, CLIENT_PACKET_SIZE);
        if (res != CLIENT_PACKET_SIZE) {
          printf("Client write rnd buffer error: %d\n", res);
	        error_flag = 1;
        }
      }
    }
    printf("=>%d ", x);
    fflush(stdout);
    x = x + 1;
    babelThreadSleep(CLIENT_SEND_DELAY);
  }
  return NULL;
}

void *receive_thread_func(void *arg) {

  int res;
  char recvbuf[SERVER_PACKET_SIZE]; 
  uint8_t d[CLIENT_PACKET_SIZE];    
  uint16_t *p;
  uint8_t *m;
  int len;
  
  for(;;) {

    if (error_flag == 0) {

      /* Receive server package
       */
      res =  babelSockReadAll(client_sock, recvbuf, SERVER_PACKET_SIZE);
      if (res != SERVER_PACKET_SIZE) {
        printf("Client read error: %d\n", res);
	      error_flag = 1;
        continue;
      }
      
      /* Decrypt each package and check for signature
       * If signature found, print message
       */
      for (int i=0; i < SERVER_MAX_NR_OF_PACKETS; i++) {

        p = (uint16_t*)(recvbuf + CLIENT_PACKET_SIZE*i);
        rsa_decrypt(d, p, CLIENT_PACKET_SIZE, g_my_private_key_d, g_my_key_n);

	      /* Check if it is for this client
	       */
        if (strncmp((char*)d, g_my_key_str, CLIENT_HEADER_SIZE) == 0) {
	        m = d + CLIENT_HEADER_SIZE;
	        len = strlen((char*)m);
	        m[len-1] = '\0';
	        printf("\n         ---(%s)---\n>>", m);
          fflush(stdout); 
        }
      }
    }
    babelThreadSleep(2000); // FIXME
  }
  return NULL;
}

int main(int argc, char *argv[]) {
    int res;
    int data;
    BabelThread_t *send_thread;
    BabelThread_t *receive_thread;
    char user_input[MESSAGE_LEN];

    /* Parse arguments
     */
    if ((argc == 6) || (argc == 7)) {
      g_my_private_key_d = atoi(argv[1]); 
      g_my_public_key_e = atoi(argv[2]); 
      g_my_key_n = atoi(argv[3]);  
      g_remote_public_key = atoi(argv[4]);  
      g_remote_n  = atoi(argv[5]);    
    } else {
      printf("Wrong number of input arguments.\n\n");
      printf("Usage: bruno_client.exe my_private_key "
	     "my_public_key my_n remote_public_key remote_n server_URL\n");
      return -1;
    }

    if (argc == 7) {
      strcpy(g_server_url, argv[6]);
    } else {
      strcpy(g_server_url, "127.0.0.1");
    }
    
    /* Initialize
     */
    send_buf_flag = 0;
    error_flag = 0;

    /* Setup headers
     */
    get_key_str(g_my_key_str, g_my_public_key_e, g_my_key_n);
    get_key_str(g_remote_key_str, g_remote_public_key, g_remote_n);
    
    /* Generate RSA keys
     */
    print_welcome();
    
    /* Connect to server
     */
    res = babelSockInit();
    if (res != BABELSOCK_OK) {
        printf("Could not initialize babelsock: %d\n", res);
        return 1;
    }
    client_sock = babelSock(BABELSOCK_TCP);
    if (client_sock < 0) {
        printf("Could not create server socket: %d\n", client_sock);
        return 1;
    }
    res = babelSockConnect(client_sock, g_server_url, SERVER_PORT);
    if (res != BABELSOCK_OK) {
        printf("Could not connect to server: %d\n", res);
        return 1;
    }
    printf("-> Connected to server!\n");
    
    /* Create and start threads
     */
    babelThreadInit();
    send_thread = babelThreadCreate(send_thread_func,
		  &data,
                  BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(send_thread);
    receive_thread = babelThreadCreate(receive_thread_func,
	                               &data,
                                       BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(receive_thread);
    printf("-> Heartbeat up and running!\n");
    printf("==========================================\n");
    
    /* Wait for input
     */
    while(1) {

      printf(">>");
      fflush(stdout); 
      fgets(user_input, TEXT_LEN, stdin);

      if (user_input[0] != '\n') {
        if (strcmp(user_input, "&genkeys\n") == 0) {
	        gen_keys();
      	} else {

          /* wait to send if busy
           */
          while (send_buf_flag != 0) {
            babelThreadSleep(50);
          }
          strcpy(g_text_buffer, user_input);
          send_buf_flag = 1;
	}
      }
    }
    babelSockClose(client_sock);
    babelSockCleanup();    
    return 0;
}
