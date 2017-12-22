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
	system("clear"); //clear the screen

	char my_username[20];

	printf("Quel sera votre surnom sur le réseau ? (max. 20 char)\n");
	fgets(my_username,20,stdin);
	strtok(my_username, "\n");
	system("clear");

	printf("Bienvenue %s, nous allons vous connecter maintenant...\n", my_username);

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

	if(send(sockfd, my_username,20,0)==-1){ // first thing to do : send the username of this client
		perror("send failed for username");
		exit(1);
	}

	int is_connected=1;
	socklen_t sin_size = sizeof(struct sockaddr_in);

	pid_t pid = fork();
	if(pid == 0){ //processus fils continue la reception des messages
		do{
			int r;
			char received_msg[1000];
			received_msg[0]='\0';
			if((r=recv(sockfd,received_msg,1000*sizeof(char),0))==-1){
				perror("reception of msg failed");
			} else if(r==0){
				break;
			} else {
				printf("\n:> %s", received_msg);
			}
			received_msg[0]='\0';
		}while(getpeername(sockfd,(struct sockaddr *)&serv_addr, &sin_size)==0);

	}
	while(is_connected){
		sleep(2);
		printf("Entrez :\n- 1 pour consulter la liste des utilisateurs connectés\n- 2 pour envoyer un message instantané\n- 3 pour quitter l’application\n");
		char choice_c;
		while((choice_c = getchar()) != '1' && choice_c != '2' && choice_c != '3'); // This will eat up all other characters

		if(choice_c == '1'){

			if(send(sockfd, "1", 4, 0)==-1){ // equivalent for asking the list of usernames to server
				perror("send failed for list");
				exit(1);
			}
		} else if (choice_c == '2'){
			system("clear");
			char msg[400];
			char tmp[20]; // recipient username
			//tmp[0]='\0';
			size_t size;
			sleep(1);

			printf("A qui envoyer (max. 20 char) : \n");
			fgets(tmp,sizeof(tmp),stdin);
			strtok(tmp, "\n");

			printf("tmp val : %s", tmp);
			sleep(5);

			printf("\nMessage à envoyer (400 char. max.) : \n");
			fgets(msg,sizeof(msg),stdin);
			strtok(msg, "\n");
			sleep(5);

			strcat(msg,":");
			strcat(msg,tmp);

			if(send(sockfd, msg, 400, 0)==-1){
				perror("send failed message");
			}
		} else if(choice_c == '3'){
			close(sockfd);
			kill(pid,SIGKILL);
			break;
		}
	}


	printf("Fermeture...");
	close(sockfd);
	exit(1);
	return 0;
}
