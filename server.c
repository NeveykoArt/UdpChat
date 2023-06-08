#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFLEN 256
#define USERNAME 15

#define CLOSE "--close\n"
#define EXIT "--exit\n"
#define LIST "--list\n"
#define HELP "--help\n"
#define CLEAR "--clear\n"

typedef struct client {
    struct sockaddr_in address;
    char username[USERNAME];
    struct client *next;
} client;

int sockfd;
client clientList;
char requestBuf[BUFLEN];
char responseBuf[BUFLEN + USERNAME];
char senderName[USERNAME];

int clientCompare(struct sockaddr_in client1, struct sockaddr_in client2) {
    if(strncmp((char *) &client1.sin_addr.s_addr, (char *) &client2.sin_addr.s_addr, sizeof(unsigned long)) == 0) {
        if(strncmp((char *) &client1.sin_port, (char *) &client2.sin_port, sizeof(unsigned short))== 0) {
            if(strncmp((char *) &client1.sin_family, (char *) &client2.sin_family, sizeof(unsigned short)) == 0) {

                return 1;
            }
        }
    }
    return 0;
}

void broadcast(struct sockaddr_in sender, int global) {
    client *cli = clientList.next;

    while(cli != NULL) {
        if (clientCompare(sender, cli->address) == 0 || global) {
            printf("Sending message to %s\n", cli->username);
            if ((sendto(sockfd, responseBuf, strlen(responseBuf), 0,(struct sockaddr *) &cli->address, sizeof(struct sockaddr))) == -1) {
                perror("sendto");
                close(sockfd);
            }
        }
        cli = cli->next;
    }
}

int isConnected(struct sockaddr_in newClient) {
    client *element = &clientList;

    while (element != NULL) {
        if (clientCompare(element->address, newClient)) {
            strncpy(senderName, element->username, USERNAME);
            printf("Client is already connected\n");
            return 1;
        }
        element = element->next;
    }
    printf("Client is not already connected\n");
    return 0;
}

int connectClient(struct sockaddr_in newClient, char *username) {
    printf("Attempting to connect client: %s\n", username);
    client *element = &clientList;

    while (element != NULL) {
        if (strcmp(element->username, username) == 0) {
            printf("Cannot connect client user already exists\n");
            strcpy(responseBuf, "");
            strcat(responseBuf, "error\n");
            if ((sendto(sockfd, responseBuf, strlen(responseBuf), 0, (struct sockaddr *) &newClient,sizeof(struct sockaddr))) == -1) {
                perror("sendto");
                close(sockfd);
                return -1;;
            }
            return -1;
        }
        element = element->next;
    }

    element = &clientList;
    while(element->next != NULL) {
        element = element->next;
    }
    element->next = malloc(sizeof(client));
    element = element->next;

    element->address = newClient;
    strncpy(element->username, username, USERNAME);
    strncpy(senderName, element->username, USERNAME);
    element->next = NULL;
    printf("Client connected\n");
    return 0;
}

int disconnectClient(struct sockaddr_in oldClient) {
    printf("Attempting to disconnect client\n");
    client *temp;
    client *element = &clientList;

    while (element->next != NULL) {
        if (clientCompare(oldClient, element->next->address)) {
            temp = element->next->next;
            free(element->next);
            element->next = temp;
            printf("Client %s disconnected\n", senderName);
            return 0;
        }
        element = element->next;
    }
    printf("Client was not disconnected properly\n");
    return -1;
}

void printClientList() {
    client *cli = clientList.next;
    int count = 1;

    while(cli != NULL) {
        printf("Client %d\n", count);
        printf("%s\n", cli->username);
        printf("ip: %s\n", inet_ntoa(cli->address.sin_addr));
        printf("port: %d\n\n", ntohs(cli->address.sin_port));
        cli = cli->next;
        count++;
    }
}

void sendClientList(struct sockaddr_in sender) {
    client *cli = clientList.next;

    while(cli != NULL) {
        if(clientCompare(sender, cli->address) == 0) {
            strcpy(responseBuf, "");
            strcpy(responseBuf, "        <");
            strcat(responseBuf, cli->username);
            strcat(responseBuf, ">");

            if ((sendto(sockfd, responseBuf, strlen(responseBuf), 0, (struct sockaddr *) &sender,sizeof(struct sockaddr))) == -1) {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        cli = cli->next;
        }
        strcpy(responseBuf, "");
        strcat(responseBuf, "\t----------\n");
        if ((sendto(sockfd, responseBuf, strlen(responseBuf), 0, (struct sockaddr *) &sender,sizeof(struct sockaddr))) == -1) {
            perror("sendto");
            close(sockfd);
            exit(EXIT_FAILURE);
    }
}

char* setSenderName(char* buf) {
  char temp[USERNAME + 10];
  bzero(temp, USERNAME + 10);
  strcat(temp,"        <");
  strcat(temp, buf);
  strcat(temp,">");
  strcpy(buf, temp);
  return buf;
}

int parseRequestBuf(char* buf) {
    char *delim1 = "<";
    char *delim2 = ">";
    client *element = &clientList;

    char bufrequest[strlen(buf)];
    bzero(bufrequest, strlen(buf));
    char *temp = strtok(buf, delim1);
    char *temp_ = strtok(temp, delim2);

    char *temp3 = strtok(senderName, delim1);
    strtok(&temp3[strlen(&temp3[0])+1], delim2);
    strcat(bufrequest, delim1);
    strcat(bufrequest, &temp3[strlen(&temp3[0])+1]);
    strcat(bufrequest, delim2);
    strcat(bufrequest, &temp_[strlen(&temp[0])+1]);

    while (element != NULL) {
        if (strcmp(element->username, &temp[0]) == 0) {
          printf("Sending message to %s\n", element->username);
            if ((sendto(sockfd, bufrequest, strlen(bufrequest), 0, (struct sockaddr *)&element->address, sizeof(struct sockaddr))) == -1) {
                perror("sendto");
                close(sockfd);
                return -1;
            }
            return 1;
        }
        element = element->next;
    }
    return 0;
}

int main(int argc, char *argv[]) {

    int server_port, nbytes;
    int address_size = sizeof(struct sockaddr_in);
    struct sockaddr_in serverAddr, clientAddr;

    bzero(requestBuf, BUFLEN);
    bzero(responseBuf, BUFLEN + USERNAME);

    if(argc != 2) {
        printf("Usage: %s portnum\n", argv[0]);
        return -1;
    }

    server_port = atoi(argv[1]);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
      close(sockfd);
      printf("Failed to get socket file descriptor\n");
      return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) == -1) {
        close(sockfd);
        printf("Failed to bind on socket!\n");
        return -1;
    }

    while(true) {
        bzero(responseBuf, BUFLEN + USERNAME);

        if ((nbytes = recvfrom(sockfd, requestBuf, BUFLEN - 1, 0, (struct sockaddr *) &clientAddr,(unsigned int *) &address_size)) == -1) {
            perror("recvfrom");
            close(sockfd);
            return -1;
        }

        requestBuf[nbytes] = '\0';
        printf("Received packet of %d bytes\n%s\n\n", nbytes, requestBuf);

        if (isConnected(clientAddr)) {
            char *sendName = senderName;
            char* buf = setSenderName(sendName);
            strcat(responseBuf, buf);
            if (strcmp(CLOSE, requestBuf) == 0) {
                if (disconnectClient(clientAddr) == 0) {
                    strcat(responseBuf, " --> disconnected" "\n");
                    broadcast(clientAddr, 1);
                    continue;
                }
                printClientList();
            }

            if (strcmp(LIST, requestBuf) == 0) {
                sendClientList(clientAddr);
                continue;
            }

            if (strcmp(HELP, requestBuf) == 0) {
              continue;
            }
            if (strcmp(CLEAR, requestBuf) == 0) {
              continue;
            }

            if (parseRequestBuf(requestBuf)) {
            } else {
                strcat(responseBuf, ": ");
                strcat(responseBuf, requestBuf);
                printf("Message: %s", responseBuf);
                printf("%s\n", senderName);
                broadcast(clientAddr, 0);
              }
        } else {
            if (connectClient(clientAddr, requestBuf) == 0) {
                char* buf2 = requestBuf;
                strcpy(buf2, requestBuf);
                char* buf = setSenderName(buf2);
                strcat(responseBuf, buf);
                strcat(responseBuf, " --> connected" "\n");
                broadcast(clientAddr, 1);
            }
            printClientList();
        }
    }
    close(sockfd);
    return 0;
}
