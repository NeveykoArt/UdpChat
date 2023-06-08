#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFLEN 256
#define USERNAME 15
#define CLOSE "--close\n"
#define LIST "--list\n"
#define HELP "--help\n"
#define CLEAR "--clear\n"

int done = 0;
int sockMain;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char sendBuf[BUFLEN];
char receiveBuf[BUFLEN + USERNAME + 2];
char userName[USERNAME];

typedef struct
{struct sockaddr_in serverAddr;} thread_data;

void menu() {

printf("\033[H\033[J");
printf("\t\t\t--Welcome <%s>  to the public chat!--\n", userName);
printf("\t\t\t\tBe cultured and polite\n\n");
printf("\t-------------------------------------------------------------------\n\n");
printf("\t*In order to see users who are online, write the command ---> --list*\n");
printf("\t*In order to disconnect from the chat, write the command ---> --close*\n");
printf("\t*In order to call help-menu to display info, write the command ---> --help*\n");
printf("\t*If you want to clear the terminal screen, write the command ---> --clear*\n");
printf("\t*If you want to write a private message before the text of the message,\n\t\t     write the username in this way ---> <username> textmessage*\n\n");
printf("\t-------------------------------------------------------------------\n");
printf("\t\t\t\t--Have a good time--\n\n\n");
}

void *sender(void *args) {

  thread_data *data = (thread_data*)args;
  struct sockaddr_in servAddr = data->serverAddr;

    while(true) {
        bzero(sendBuf, BUFLEN);
        fgets(sendBuf, BUFLEN, stdin);

        if (strcmp(sendBuf, LIST) == 0) {
            printf("\t--ONLINE--\n");
            printf("\t<%s>(You)\n", userName);
        }

        if (sendto(sockMain, sendBuf, strlen(sendBuf), 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1)  {
            perror("send");
            done = 1;
            pthread_mutex_destroy(&mutex);
            pthread_exit(NULL);
        }

        if (strcmp(sendBuf, HELP) == 0) {
            menu();
        }

        if (strcmp(sendBuf, CLEAR) == 0) {
            printf("\033[H\033[J");
        }

        if (strcmp(sendBuf, CLOSE) == 0) {
            done = 1;
            pthread_mutex_destroy(&mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&mutex);
    }
}

void *receiver(void *args) {
    int nbytes;
    thread_data *data = (thread_data*)args;
    struct sockaddr_in servAddr = data->serverAddr;
    socklen_t length = sizeof(struct sockaddr_in);

    while(true) {
        bzero(receiveBuf, BUFLEN);

        if ((nbytes = (recvfrom(sockMain, receiveBuf, BUFLEN - 1, 0, (struct sockaddr *)&servAddr, &length))) == -1) {
            perror("recv");
            done = 1;
            pthread_mutex_destroy(&mutex);
            pthread_exit(NULL);
        }

        receiveBuf[nbytes] = '\0';
        if (strcmp("error\n", receiveBuf) == 0) {
            printf("Error: The username is already taken.\n");
            done = 1;
            pthread_mutex_destroy(&mutex);
            pthread_exit(NULL);
        }
        else {
            printf("%s\n", receiveBuf);
            pthread_mutex_unlock(&mutex);
        }
    }
}

int main(int argc, char *argv[]) {
    bzero(sendBuf, BUFLEN);
    bzero(receiveBuf, BUFLEN + USERNAME + 2);

    int portnum;

    if(argc != 4) {
        fprintf(stderr, "Usage: %s [server] [portnum] [username]\n", argv[0]);
        return -1;
    }

    portnum = atoi(argv[2]);
    strncpy(userName, argv[3], USERNAME);

    struct hostent *host;

    if ((host = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "Failed to resolve server host information\n");
        return -1;
    }

    struct sockaddr_in servAddr;

    if ((sockMain = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        close(sockMain);
        fprintf(stderr, "Failed to get socket file descriptor\n");
        return -1;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portnum);
    servAddr.sin_addr = *((struct in_addr *) host->h_addr);

    strcpy(sendBuf, userName);

    if (sendto(sockMain, sendBuf, strlen(sendBuf), 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
        perror("send");
        close(sockMain);
        return -1;
    }

    pthread_t threads[2];
    pthread_attr_t attr;

    thread_data data = {servAddr};

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    menu();

    pthread_create(&threads[0], &attr, (void *)sender, (void *)&data);
    pthread_create(&threads[1], &attr, (void *)receiver, (void *)&data);

    while (!done);

    close(sockMain);
    return 0;
}
