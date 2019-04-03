#include "comm.h"


int client_work(int argc, char* argv[])
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket");
        return 2;
    }

    struct sockaddr_in server_socket;
    //struct sockaddr_in client_socket;
    server_socket.sin_family = AF_INET;
    server_socket.sin_addr.s_addr = inet_addr(argv[1]);
    server_socket.sin_port = htons(atoi(argv[2]));

    int ret = connect(server_fd, (struct sockaddr*)&server_socket, sizeof(server_socket));
    if(ret < 0){
        perror("connect");
        return 3;
    }

    printf("%s connect success...\n", argv[3]);

    if(fcntl(0, F_SETFL, FNDELAY) < 0){
        perror("fcntl");
        return 4;
    }

    Msg msg;
    for(;;){
        memset(&msg, 0, sizeof(msg));
        strcpy(msg.name, argv[3]);
        int read_size = read(0, msg.data, sizeof(msg.data));
        if(read_size > 0){
            msg.data[read_size-1] = '\0';
            send(server_fd, &msg, sizeof(msg), MSG_DONTWAIT);
        }

        int recv_size = recv(server_fd, &msg, sizeof(msg), MSG_DONTWAIT);
        if(recv_size > 0){
            printf("\t\t%s say : %s\n", msg.name, msg.data);
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc != 4){
        printf("Usage ./server [ip] [port] [name]\n");
        return 1;
    }
    client_work(argc, argv);

    return 0;
}
