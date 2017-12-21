#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT 5555
#define BACKLOG 10
#define MAX_SIZE_R 10000

char dst[MAX_SIZE_R]; // global variable

struct clientStruct{
  char* ipaddr;
  char* username;
  int sockfd;
};

struct clientList{
	struct clientStruct * list; //tableau de vecteurs ["192.168.1.20", "Fayçal"]
	int size; // current size of list
  int remaining; // remaining space
};

int searchClientByUsername(struct clientList *clients, char* username){
  int max_iter = clients->size;
  for(int i = 0; i<max_iter; i++){
    if(clients->list[i].username == username){
      return clients->list[i].sockfd;
    }
  }
  return -1;
}

char* getClientsList(struct clientList *clients){
  char buffer[MAX_SIZE_R] = "\nList of connected clients : \n";
  char src[25];
  char cst1[]="- ";
  char cst2[]="\n";

  char si = clients->size;
	for(int i=0; i < si ;i++){
    strcpy(dst,buffer);
    strcpy(src,cst1);
    strcat(src,clients->list[i].username);
    strcat(src,cst2);
    strcat(dst,src);
	}
  return dst;
}

int insertClient(struct clientList *clients, struct clientStruct *newClient){

  if(searchClientByUsername(clients, newClient->username) == -1){// surnom déjà utilisé
    (*clients).list[clients->size] = *newClient;
    clients->remaining -=1;
    clients->size +=1;
    //printClientsList(clients);
    return 1;
  } else {
    return 0;
  }
}

int deleteClient(struct clientList *clients, int index){
  if(index >= clients->size){
    return 0;
  } else {
    for(int i = index-1; i<clients->size-1; i++){
      clients->list[i] = clients->list[i+1];
    }
    clients->size -= 1;
    return 1;
  }
}

int main(void){
	struct clientList connectedClients;
  connectedClients.list = (struct clientStruct *) malloc(sizeof(struct clientStruct));
	connectedClients.size = 0;
  connectedClients.remaining=1;
	// initiliaze useful variables
	int new_sfd;
	struct sockaddr_in connector_addr;

	// create socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed");
		exit(1);
	} else {
    printf("socket success \n");
  }

	// create a sockaddr
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero),'\0',8);
  printf("sockaddr \n");

	// show host ip addr
	//printf("Hostname: %s", inet_ntoa(my_addr.sin_addr));

	// bind the socket to my_addr
	int b = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	if(b == -1){
		perror("bind failed");
		exit(1);
	} else {
    printf("binding success \n");
  }
	// listen on the socket
	int l = listen(sockfd, BACKLOG);
	if(l == -1){
		perror("listen failed");
		exit(1);
	} else {

    printf("Listening...\n");
  }

	while(1){
		socklen_t sin_size = sizeof(struct sockaddr_in);
		new_sfd = accept(sockfd, (struct sockaddr *)&connector_addr, &sin_size);
    if(new_sfd == -1){
			perror("accept failed");
			exit(1);
		}
		printf("\nServer got connection from : %s", inet_ntoa(connector_addr.sin_addr));

    char username[20];
    if(recv(new_sfd, &username, 101,0)==-1){ // receive the username
      perror("reception failed of username");
    }
    char welcome[]="\nArbFay's Messenger va maintenant commencer. Votre nom en ligne est ";
    strcat(welcome, username);
    strcat(welcome, ".\n");

    printf("%s",welcome);

    if(send(new_sfd, welcome, strlen(welcome),0) == -1){
      perror("send welcome failed");
    }

    //Create a 'clientStruct' with ipaddr and username and add it to the list
    struct clientStruct new_client;
    new_client.ipaddr = inet_ntoa(connector_addr.sin_addr);
    new_client.username = username;
    new_client.sockfd = new_sfd;

    if(insertClient(&connectedClients, &new_client) < 1){
        perror("client insertion failed");
    }

		if(!fork()){ // Fork for managing the messaging

      // ------ Manage the selection of who to send message to in the client app -------

      int is_connected=1;

      do{
        char* message=(char*)malloc(sizeof(char)*400);
        if(recv(new_sfd, message, 400,0)==-1){ // receive the username
          perror("reception failed of message");
        }

        if(strcmp(message, "list")==0){
          message=getClientsList(&connectedClients);
          if(send(new_sfd, message, strlen(message),0)==-1){// send the list of clients
            perror("send client list failed");
          }
        } else {
          // the message sent by clients is constructed this way :
          // [recipient's name]: [message]
          // split the received message with the data and send it in this structure:
          // [sender's name]: [message]
          char* recipient_username;
          char* sender_username="";
          char* msg_tosend="";
          char* final_msg="";

          strcat(sender_username,new_client.username);
          strcat(sender_username, ": ");

          char* buf = strtok(message, ":");
          recipient_username = buf;

          while(buf != NULL){
            strcat(msg_tosend,strtok(NULL, ":"));
          }

          strcat(final_msg, sender_username);
          strcat(final_msg,msg_tosend);

          // send the message to the right user
          write(searchClientByUsername(&connectedClients, recipient_username), final_msg, strlen(msg_tosend));
          sleep(1);
          /*if(send(searchClientByUsername(&connectedClients, recipient_username), final_msg, strlen(msg_tosend),0) == -1){
            perror("send msg failed");
          }*/
        }
        free(message);
      }while(is_connected);

			close(new_sfd);
			exit(0);
		}
	}
  return 0;
}
