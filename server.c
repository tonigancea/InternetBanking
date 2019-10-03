#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFLEN 256

typedef struct {
    char nume[13];
    char prenume[13];
    int numar_card;
    int pin;
    char parola_secreta[9];
    double sold;
    int blocked;
} user;

typedef struct {
    int status;
    int fd;
    int card;
} logged;

void error(char *msg) {
    perror(msg);
    exit(1);
}

void showUsers(user* users, int n) {
    for (int i = 0; i < n; i++) {
        printf("%s %s %d %d %s %.2lf\n", users[i].nume, users[i].prenume,
                users[i].numar_card, users[i].pin, users[i].parola_secreta,
                users[i].sold);
    }
}

void init(logged *loggedPersons, int *mistakes, int MAX) {
    for (int i = 0; i < MAX; i++) {
        loggedPersons[i].status = 0;
        loggedPersons[i].fd = -1;
        loggedPersons[i].card = 0;
        mistakes[i] = 0;
    }
}

int getFirstFree(logged *loggedPersons, int MAX) {
    for (int i = 0; i < MAX; i++) {
        if (loggedPersons[i].status == 0) {
            return i;
        }
    }
    return -1;
}

int checkIfCardIsOpen(int card, logged *loggedPersons, int MAX) {
    for (int i = 0; i < MAX; i++) {
        if (loggedPersons[i].status == 1) {
            if (loggedPersons[i].card == card) {
                return 1;   //card is opened
            }
        }
    }
    return 0;
}

int incrementFailAndCheckLock(int i, int *mistakes, char *buffer, user *users, int n) {
    mistakes[i] += 1;
    if (mistakes[i] == 3) {
        users->blocked = 1;
        memset(buffer, 0, BUFLEN);
        sprintf(buffer,"iBANK> -5 : Card blocat");
        send(i,buffer,BUFLEN,0);
        return 0;
    }
    return 1;
}

void login(int i, logged *loggedPersons, user *users, int *mistakes, char *buffer, int n, int MAX) {
    char *tok;
    tok = strtok(buffer, " "); //ignorring
    tok = strtok(NULL, " ");
    int card = atoi(tok);
    tok = strtok(NULL, " ");
    int pin = atoi(tok);

    for (int j = 0; j < n; j++) {
        if (users[j].numar_card == card) {
            if (users[j].blocked == 1) {
                memset(buffer, 0, BUFLEN);
                sprintf(buffer,"iBANK> -5 : Card blocat");
                send(i,buffer,BUFLEN,0);
                return;
            }
            if (users[j].pin != pin) {
                if (incrementFailAndCheckLock(i,mistakes,buffer,&users[j],n) > 0) {
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer,"iBANK> -3 : Pin gresit");   //de facut treaba cu blocatul la 3 greseli
                    send(i,buffer,BUFLEN,0);
                    return;
                }
            }
            if (users[j].pin == pin) {
                if (checkIfCardIsOpen(card, loggedPersons, MAX) == 0) {
                    int pos = getFirstFree(loggedPersons, MAX);
                    loggedPersons[pos].status = 1;
                    loggedPersons[pos].fd = i;
                    loggedPersons[pos].card = card;
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer,"iBANK> Welcome %s %s", users[j].nume, users[j].prenume);
                    send(i,buffer,BUFLEN,0);
                    return;
                } else {
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer,"iBANK> -2 : Sesiune deja deschisa");
                    send(i,buffer,BUFLEN,0);
                    return;
                }
            }
        }
    }
    memset(buffer, 0, BUFLEN);
    sprintf(buffer,"iBANK> -4 : Numar card inexistent");
    send(i,buffer,BUFLEN,0);
}

void logout(int i, char *buffer, logged *loggedPersons, int MAX) {
    for (int j = 0; j < MAX; j++) {
        if (loggedPersons[j].status == 1 && loggedPersons[j].fd == i) {
            loggedPersons[j].status = 0;
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "iBANK> Clientul a fost deconectat");
            send(i, buffer, BUFLEN, 0);
            return;
        }
    }
}

void listSold(int i, char *buffer, logged *loggedPersons, user *users, int MAX, int n) {
    
    int card = -1;
    for (int j = 0; j < MAX; j++) {
        if (loggedPersons[j].status == 1 && loggedPersons[j].fd == i) {
            card = loggedPersons[j].card;
        }
    }
    for (int k = 0; k < n; k++) {
        if (users[k].numar_card == card) {
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "iBANK> %.2f", users[k].sold);
            send(i, buffer, BUFLEN, 0);
        }
    }
}

double findMySold(int i, user *users, logged *loggedPersons, int MAX, int n) {
    
    int myCard = -1;
    for (int j = 0; j < MAX; j++) {
        if (loggedPersons[j].status == 1 && loggedPersons[j].fd == i) {
            myCard = loggedPersons[j].card;
        }
    }
    for (int j = 0; j < n; j++) {
        if (users[j].numar_card == myCard) {
            return users[j].sold;
        }
    }
    return 1;
}

void transfer(int i, char *buffer, user *users, logged *loggedPersons, int MAX, int n) {
    char *tok;
    tok = strtok(buffer, " "); //ignorring
    tok = strtok(NULL, " ");
    int destinatar = atoi(tok);
    tok = strtok(NULL, " ");
    double suma = 0;
    sscanf(tok,"%lf",&suma);

    int index = -1;

    for (int j = 0; j < n; j++) {
        if (users[j].numar_card == destinatar) {
            double mySold = findMySold(i,users,loggedPersons,MAX,n);
            if (mySold < suma) {
                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "iBANK> -8 : Fonduri insuficiente");
                send(i, buffer, BUFLEN, 0);
                return;
            } else {
                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "iBANK> Transfer %.2f catre %s %s? [y/n]",
                        suma, users[j].nume, users[j].prenume);
                send(i, buffer, BUFLEN, 0);

                memset(buffer, 0, BUFLEN);
                recv(i, buffer, BUFLEN, 0);
                if (buffer[0] == 'y') {

                    int myCard = -1;
                    for (int k = 0; k < MAX; k++) {
                        if (loggedPersons[k].status == 1 && loggedPersons[k].fd == i) {
                            myCard = loggedPersons[k].card;
                            break;
                        }
                    }

                    for (int k = 0; k < n; k++) {
                        if (users[k].numar_card == myCard) {
                            index = k;
                            break;
                        }
                    }

                    users[index].sold -= suma;
                    users[j].sold += suma;

                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "iBANK> Transfer realizat cu sucess");
                    send(i, buffer, BUFLEN, 0);
                    return;
                } else {
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "iBANK> -9 : Operatie anulata");
                    send(i, buffer, BUFLEN, 0);
                    return;
                }
            }
        }
    }

    memset(buffer, 0, BUFLEN);
    sprintf(buffer,"iBANK> -4 : Numar card inexistent");
    send(i,buffer,BUFLEN,0);
}

int checkCardIfExists(int card, user *users, int n) {
    for (int i = 0; i < n; i++) {
        if (users[i].numar_card == card) {
            return 1;
        }
    }
    return 0;
}

int checkCardIfLocked(int card, user *users, int n) {
    for (int i = 0; i < n; i++) {
        if (users[i].numar_card == card && users[i].blocked == 1) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
     
     int n = 0, i, j, MAX = 0, check, sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     fd_set read_fds;
     fd_set tmp_fds;	
     int fdmax;

     FILE *file = fopen(argv[2], "r");
     char *line;
     size_t len = 0;
     int count;
     if ((count = getline(&line, &len, file)) < 0) {
        error("Cannot read line.");
     }
     sscanf(line, "%d", &n);
     MAX = 3*n;
     int mistakes[MAX];

     logged loggedPersons[MAX];
     init(loggedPersons,mistakes,MAX);

     user users[n];
     memset(users, 0, n*sizeof(user));

     for (int i = 0; i < n; i++) {
        if ((count = getline(&line,&len,file)) < 0) {
            error("Cannot read line.");
        } 
        sscanf(line, "%s %s %d %d %s %lf", users[i].nume, users[i].prenume,
               &(users[i].numar_card), &(users[i].pin), users[i].parola_secreta,
               &(users[i].sold));
     }
     fclose(file);

     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     portno = atoi(argv[1]);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
              error("ERROR on binding");

     listen(sockfd, MAX);
     FD_SET(sockfd, &read_fds);
     FD_SET(0,&read_fds);
     
     //udp
     struct sockaddr_in serv_addr_udp;
     int fd_udp = socket(AF_INET,SOCK_DGRAM,0);
     if (fd_udp < 0)
        error("ERROR opening socket udp");

     serv_addr_udp.sin_family = AF_INET;
     serv_addr_udp.sin_port = htons(portno);
     serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

     if (bind(fd_udp, (struct sockaddr *) &serv_addr_udp, sizeof(struct sockaddr)) < 0)
              error("ERROR on binding udp");

     FD_SET(fd_udp, &read_fds);
     fdmax = fd_udp;

	while (1) {
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					}
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) {
							fdmax = newsockfd;
						}
					}
					printf("[MENTENANCE] Noua conexiune de la %s, port %d, socket_client %d\n",
                        inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

				} else if (i == 0) {
                    memset(buffer, 0, BUFLEN);
                    if ((check = read(0, buffer, sizeof(buffer))) <= 0) {
                        printf("error receiving from stdin\n");
                        mistakes[i] = 0;
                        close(i);
                        FD_CLR(i,&read_fds);
                    } else {
                        if (strncmp(buffer,"quit",4) == 0) {
                            for (int j = 1; j <= fdmax; j++) {
                                if (j != sockfd) {  //aici sa fie diferit si de socketul udp cred
                                    memset(buffer, 0, BUFLEN);
                                    sprintf(buffer, "[MENTENANCE]> Serverul se inchide");
                                    send(j, buffer, BUFLEN, 0);
                                    close(j);
                                    FD_CLR(j,&read_fds);
                                }
                            }
                            close(sockfd);
                            return 0;
                        }
                    }
                // } else if (i == fd_udp) {
                //         printf("I am here\n");
                //         struct sockaddr_in from_client;
                //         socklen_t len;
                //         recvfrom(fd_udp,buffer,BUFLEN,0,(struct sockaddr*)&from_client,&len);
                //         int card_to_check;
                //         char tmp[10];
                //         printf("%s\n",buffer);
                //         sscanf(buffer,"%s %d\n",tmp,&card_to_check);
                //         printf("%d\n",card_to_check);
                //         if (checkCardIfExists(card_to_check, users, n) == 1) {
                //             printf("I am here3\n");
                //             memset(buffer, 0, BUFLEN);
                //             sprintf(buffer,"UNLOCK> Trimite parola secreta");
                //             sendto(fd_udp,buffer,BUFLEN,0,(struct sockaddr*)&serv_addr_udp, sizeof(serv_addr_udp));   
                //         }
                //         printf("I am here2\n");
                //         memset(buffer, 0, BUFLEN);
                //         sprintf(buffer,"UNLOCK> -4 : Numar card inexistent");
                //         sendto(fd_udp,buffer,BUFLEN,0,(struct sockaddr*)&serv_addr_udp, sizeof(serv_addr_udp));

                } else {
					//am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((check = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                        error("ERROR in recv");
                        mistakes[i] = 0;
                        close(i);
                        FD_CLR(i,&read_fds);
					} else { 
                        char backupBuffer[BUFLEN];
                        strcpy(backupBuffer,buffer);
                        
                        char *tok;
                        tok = strtok(backupBuffer," \n");

                        if (strcmp(tok,"login") == 0) {
                            login(i, loggedPersons, users, mistakes, buffer, n, MAX);
                        }

                        if (strcmp(tok,"logout") == 0) {
                            logout(i, buffer, loggedPersons, MAX);
                        }

                        if (strcmp(tok,"listsold") == 0) {
                            listSold(i, buffer, loggedPersons, users, MAX, n);
                        }

                        if (strcmp(tok,"transfer") == 0) {
                            transfer(i, buffer, users, loggedPersons, MAX, n);
                        }

                        if (strcmp(tok,"quit") == 0) {
                            mistakes[i] = 0;
                            close(i); 
                            FD_CLR(i, &read_fds);
                        }
                    }
                }
			}
		}
	}
    close(sockfd);
    return 0;
}
