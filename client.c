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
#include <netdb.h>

#define MYPORT 5555
#define BACKLOG 10

int main(int argc, char* argv[]){
	// ask username

	system("clear");

	char my_username[20];
	char recipient_username[20];

	printf("Quel sera votre surnom sur le réseau ? (max. 20 char)\n");
	fgets(my_username,20,stdin);
	strtok(my_username, "\n");
	system("clear");

	printf("Bienvenue %s, nous allons nous connecter maintenant...\n", my_username);

	// create socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed");
		exit(1);
	}

	// create a sockaddr
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero),'\0',8);

	// create server sockaddr
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(MYPORT);
	inet_aton(argv[1],&(serv_addr.sin_addr));
	memset(&(serv_addr.sin_zero),'\0',8);

	int c = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	if(c == -1){
		perror("connect failed");
		exit(1);
	}

	if(send(sockfd, my_username,20,0)==-1){ // first : send the username of this client
		perror("send failed for username");
		exit(1);
	}

	int is_connected=0;
	do{
		char buffer[10000];


		if(send(sockfd, "list",4,0)==-1){ // equivalent for asking the list of usernames to server
			perror("send failed for list");
			exit(1);
		}

		int r = recv(sockfd, &buffer, 10000,0); // receives the list of usernames who we can text
		if(r ==-1){
			perror("recv failed");
			exit(1);
		}
		sleep(5);
		printf("%s\n",buffer);// print username

		printf("Entrez le nom de la personne avec qui vous souhaitez communiquer : ");
		fgets(recipient_username,20,stdin);
		strtok(recipient_username,"\n");
		// verify if the entered name is in the list
		char *tmp = strstr(buffer, recipient_username);
		printf("%s",tmp);

		if(tmp){
			is_connected=1;
		} else {
			printf("Mauvais nom d'utilisateur !");
		}
	}while(!is_connected); // continue to ask to

	socklen_t sin_size = sizeof(struct sockaddr_in);
	system("clear");

	pid_t pid = fork();
	if(pid == 0){
		//keep receiving messages here
		printf("-----Début du chat avec %s-----\n",recipient_username);
		do{
			char received_msg[400];
			/*if(recv(sockfd, &received_msg, 400,0)){
				perror("reception failed message");
			}*/
			read(sockfd,received_msg,sizeof(received_msg));
			printf(":> %s", received_msg);
			printf("\n");
			sleep(1);
		}while(getpeername(sockfd,(struct sockaddr *)&serv_addr, &sin_size)==0);

	} else if(pid != -1) {
		//send messages here
		sleep(1);
		strtok(recipient_username,"\n");
		do{
			char tosend_msg[400];
			printf(":> for %s : ", recipient_username);
			fgets(tosend_msg,400,stdin);
			strtok(my_username, "\n");
			if(send(sockfd, tosend_msg, 400, 0)==-1){
				perror("send failed message");
			}
		}while(getpeername(sockfd,(struct sockaddr *)&serv_addr, &sin_size)==0);
	} else {
		perror("Erreur durant le fork");
		exit(1);
	}
	printf("Fermeture...");
	close(sockfd);

	return 0;
}
