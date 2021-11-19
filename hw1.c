#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <sys/wait.h>

#define SOCKET int

struct {
    char *ext;
    char *filetype;
} extensions [] = {{"jpg", "image/jpeg"}, {"jpeg","image/jpeg"}, {"gif", "image/gif" }, 
                    {"png", "image/png" }, {"zip", "image/zip" }, {"gz",  "image/gz"  },
                    {"tar", "image/tar" }, {"htm", "text/html" }, {"html","text/html" },
                    {"exe","text/plain" }, {0,0} };


static void sigchld_handler()
{
    pid_t PID;
    int status;
    
    while (PID = waitpid(-1, &status, WNOHANG) > 0)
        printf("child process %d end...\n", PID);
        
    signal(SIGCHLD, sigchld_handler);
}

void handle_socket(SOCKET client)
{

    int fdIMG, buflen, len, ret;
    char * fstr;
    char buf[2048];
    memset(buf, 0, 2048);

    ret = read(client,buf,2047);
    //ret = recv(client, buf, 2047, 0);
    printf("buf:%s\n", buf);
    //printf("buf_l:%d\n", ret);

    if ((ret>0) && ((ret<2047)))
        buf[ret] = 0;
    else
        buf[0] = 0;
 
    for (int i=0;i<ret;i++) {
        if (buf[i]=='\r'||buf[i]=='\n')
            buf[i] = 0;
    }
    
    //GET
    if ( (!strncmp(buf,"GET ",4)) || (!strncmp(buf,"get ",4)) ){

        for(int i = 4; i < 2047; i++) {
            if(buf[i] == ' ') {
                buf[i] = 0;
                break;
            }
        }

        if (!strncmp(&buf[0],"GET /\0",6)||!strncmp(&buf[0],"get /\0",6) )
            strcpy(buf,"GET /index.html\0");

        buflen = strlen(buf);

        for(int i = 0; extensions[i].ext != 0; i++) {
            len = strlen(extensions[i].ext);
            if(!strncmp(&buf[buflen-len], extensions[i].ext, len)) {
                fstr = extensions[i].filetype;
                break;
            }
        }

        fdIMG=open(&buf[5], O_RDONLY);

        sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
        //write(client, buf, strlen(buf));

        while ((ret = read(fdIMG, buf, 2047))>0) {
            write(client, buf, strlen(buf));
        }
    }

    close(client);
    printf("close...\n");
}


int main(int argc, char *argv[])
{

    struct sockaddr_in ser_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    SOCKET server, client;
    int on = 1;
    pid_t PID;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if(server < 0){
        perror("socket");
        exit(1);
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = INADDR_ANY;
    ser_addr.sin_port = htons(8080);

    signal(SIGCHLD, sigchld_handler);

    if(bind(server, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1){
        perror("bind");
        close(server);
        exit(1);
    }

    if(listen(server, 10) == -1){
        perror("listen");
        close(server);
        exit(1);
    }


    while(1){
        client = accept(server, (struct sockaddr *)&cli_addr, &cli_len);

        if(client == -1){
            perror("Connection failed...\n");
            continue;
        }

        printf("Got client connection...\n");

        PID = fork();
        if(PID == 0){
            //child
            close(server);
            handle_socket(client);

            exit(0);
        }
        //parent
        close(client);
    }

    return 0;
}
