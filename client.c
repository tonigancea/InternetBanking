#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void writeComms(int fd, char *buffer1, char *buffer2) {
    printf("%s\n",buffer2);
    char buffer_tmp[BUFLEN];
    memset(buffer_tmp, 0, BUFLEN);
    sprintf(buffer_tmp, "%s\n", buffer2);
    
    write(fd, buffer1, BUFLEN);
    write(fd, buffer_tmp, BUFLEN);
}

int main(int argc, char *argv[])
{
    int sockfd, sockfd_udp, n, i;
    struct sockaddr_in serv_addr, serv_addr_udp;
    struct hostent *server;
    char buffer[BUFLEN];
    char buffer_recv[BUFLEN];
    char tmp[BUFLEN];
    int status = 0;
    int check;

    int last_card;
    int last_pin;
	
    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    char filename[30];
    sprintf(filename,"client-%d.log",getpid());
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_udp < 0) {
        error("ERROR opening socketUdp");
    }
    serv_addr_udp.sin_family = AF_INET;
    serv_addr_udp.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr_udp.sin_addr);
    if (connect(sockfd_udp,(struct sockaddr*) &serv_addr_udp,sizeof(serv_addr_udp)) < 0)
        error("ERROR connecting");

    FD_SET(sockfd, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(0,&read_fds);
    fdmax = sockfd;

    int waitforpass = 0;

    while (1) {

        tmp_fds = read_fds;
        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            error("ERROR in select");

        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == 0) {
                    //all from stdin
                    memset(buffer, 0 , BUFLEN);
                    memset(buffer_recv, 0, BUFLEN);
                    read(i, buffer, BUFLEN);

                    sprintf(tmp,"%s",buffer);
                    char *tok = NULL;
                    tok = strtok(tmp," \n");

                    // if (waitforpass == 1) {
                    //     //send the pass
                    // }

                    if (strncmp(tok,"unlock",6) == 0) {
                        char tmp[BUFLEN];
                        memset(tmp, 0, BUFLEN);
                        sprintf(tmp,"%s %d","unlock",last_card);
                        sendto(sockfd_udp, tmp, BUFLEN, 0, (struct sockaddr*)&serv_addr_udp, sizeof(serv_addr_udp));
                        continue;
                        
                    }

                    if (strcmp(tok,"login") == 0 && status == 1) {
                        sprintf(buffer_recv,"-2 : Sesiune deja deschisa");
                        writeComms(fd,buffer,buffer_recv);
                        continue;
                    
                    } else {
                        
                        if (strcmp(tok,"logout") == 0 && status == 0) {
                            sprintf(buffer_recv,"-1 : Clientul nu este autentificat");
                            writeComms(fd,buffer,buffer_recv);
                            continue;
                        }

                        if (strcmp(tok,"logout") == 0 && status == 1) {
                            status = 0;
                        }

                        if (strcmp(tok,"login") == 0) {
                            char tmp[30];
                            sscanf(buffer,"%s %d %d", tmp, &last_card, &last_pin);
                            printf("%d %d\n",last_card, last_pin);
                        }

                        n = send(sockfd,buffer,BUFLEN, 0);
                        if (n < 0)
                             error("ERROR writing to socket");

                        if (strcmp(tok,"quit") == 0) {
                            write(fd, buffer, BUFLEN);
                            close(fd);
                            return 0;
                        }

                        n = recv(sockfd,buffer_recv,BUFLEN,0);

                        if (n < 0)
                             error("ERROR reciving back");

                        sprintf(tmp,"%s",buffer_recv);
                        tok = strtok(tmp," \n");
                        tok = strtok(NULL," \n");

                        if (strcmp(tok,"Welcome") == 0) {
                            status = 1;
                        }
                    }

                    writeComms(fd,buffer,buffer_recv);
                    
                } else if (i == sockfd) {
                    //from server
                    memset(buffer_recv, 0, BUFLEN);
                    if ((check = recv(sockfd, buffer_recv, BUFLEN, 0)) < 0) {
                        error("ERROR in reciving from server");
                        close(i);
                        FD_CLR(i,&read_fds);
                    } else {
                        sprintf(tmp,"%s",buffer_recv);
                        char *tok = NULL;
                        tok = strtok(tmp," \n");

                        if(strncmp(tok,"[MENTENANCE]",12) == 0) {
                            printf("%s\n",buffer_recv);
                            sprintf(buffer_recv,"%s\n",buffer_recv);
                            write(fd, buffer_recv, BUFLEN);
                            close(i);
                            FD_CLR(i,&read_fds);
                            close(fd);
                            return 0;
                        }
                    }
                // } else if (i == sockfd_udp) {
                //     memset(buffer_recv, 0, BUFLEN);
                //     struct sockaddr_in from_server;
                //     socklen_t len;
                //     recvfrom(sockfd_udp,buffer_recv,BUFLEN,0,(struct sockaddr*)&from_server,&len);
                //     sprintf(tmp,"%s",buffer_recv);
                //     char *tok = NULL;
                //     tok = strtok(tmp," \n");
                //     tok = strtok(NULL," \n");
                //     if (strncmp(tok,"Trimite",7) == 0) {
                //         waitforpass = 1;
                //     }
                }
            }
        }    	 
    }
    close(fd);
    return 0;
}
