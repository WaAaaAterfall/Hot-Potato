#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "potato.h"
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>


int connect_to_read_fds(const char* hostname, const char *portNumber){
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, portNumber, &host_info, &host_info_list);
    if (status != 0) {
        fprintf(stderr, "Error: cannot get address info for host\n");
        return -1;
    }

    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        fprintf(stderr, "Error: cannot create socket\n");
        return -1;
    }
    
    printf("Connecting to %s, on portNumber %s ...\n", hostname, portNumber);
    
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        fprintf(stderr, "Error: cannot connect to socket\n");
        return -1;
    }
    freeaddrinfo(host_info_list);
    return socket_fd;
}

int get_port_number(int socket_fd){
    struct sockaddr_in socket_info_name;
    socklen_t name_len = sizeof(socket_info_name);
    int error = getsockname(socket_fd, (struct sockaddr*)&socket_info_name, &name_len);
    if(error == -1){
        fprintf(stderr, "Error: cannot get socket info\n");
        return -1;
    }
    int port = ntohs(socket_info_name.sin_port);
    return port;

}

int setup_socket_for_right(int playerId, int playerNum){//, const char* portNumber){
    int status_server;
    int socket_fd_right;
    struct addrinfo host_info_server;
    struct addrinfo *host_info_list_server;

    memset(&host_info_server, 0, sizeof(host_info_server));
    host_info_server.ai_family   = AF_UNSPEC;
    host_info_server.ai_socktype = SOCK_STREAM;
    host_info_server.ai_flags = AI_PASSIVE;
    char * hostname_server = NULL;
    int port_right = 0;//atoi(portNumber) + playerId + 1;
    //sprintf(portNumber_right, "%d", port_right);

    status_server = getaddrinfo(hostname_server, "", &host_info_server, &host_info_list_server);
    
    if (status_server != 0) {
        fprintf(stderr, "Error: cannot get address info for current server\n");
        return -1;
    }

    socket_fd_right = socket(host_info_list_server->ai_family, 
                host_info_list_server->ai_socktype, 
                host_info_list_server->ai_protocol);

    if (socket_fd_right == -1) {
        fprintf(stderr, "Error: cannot create socket for right player\n");
        return -1;
    }    
    int yes = 1;
    status_server = setsockopt(socket_fd_right, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status_server = bind(socket_fd_right, host_info_list_server->ai_addr, host_info_list_server->ai_addrlen);
    if (status_server == -1) {
        fprintf(stderr, "Error: cannot bind socket for right player\n");
        return -1;
    } 
    status_server = listen(socket_fd_right, 100);
    if (status_server == -1) {
        fprintf(stderr, "Error: cannot listen to socket for right player\n");
        return -1;
    }
    freeaddrinfo(host_info_list_server);
    return socket_fd_right;
}

/** receive and accpet the connect from right player**/
int accept_connect_from_right(int socket_fd_right, int playerId, int playerNum){
    struct sockaddr_storage socket_addr_server;
    socklen_t socket_addr_len_server = sizeof(socket_addr_server);
    int player_right_fd = accept(socket_fd_right, (struct sockaddr *)&socket_addr_server, &socket_addr_len_server);
    if (player_right_fd == -1) {
        printf("Error: cannot connect to the right player socket\n");
        return -1;
    }
    char confirm[512];
    if(playerId != playerNum - 1){
        sprintf(confirm, "Player %d is connected with player %d\n", playerId, playerId + 1);
    }else{
        sprintf(confirm, "Player %d is connected with player %d\n", playerId, 0);
    }
    //send(player_right_fd, confirm, strlen(confirm), 0);
    return player_right_fd;
}

/**** Connect with the player to the left by sending connect request like a client*****/
int connect_to_left(int playerId, int playerNum, int port_left, const char * hostname_left){
    int status_left;
    int socket_fd_left;
    struct addrinfo host_info_left;
    struct addrinfo *host_info_list_left;

    memset(&host_info_left, 0, sizeof(host_info_left));
    host_info_left.ai_family   = AF_UNSPEC;
    host_info_left.ai_socktype = SOCK_STREAM;
    host_info_left.ai_flags = AI_PASSIVE;

    // int port_left = 0;
    // if(playerId != 0){
    //     port_left = atoi(portNumber) + playerId;
    // }else{
    //     port_left = atoi(portNumber) + playerNum;
    // }
    char portNumber_left[512];
    sprintf(portNumber_left, "%d", port_left);

    status_left = getaddrinfo(hostname_left, portNumber_left, &host_info_left, &host_info_list_left);
        
    if (status_left != 0) {
        fprintf(stderr, "Error: cannot get address info for left player\n");
        return -1;
    }

    socket_fd_left = socket(host_info_list_left->ai_family, 
                host_info_list_left->ai_socktype, 
                host_info_list_left->ai_protocol);
    if (socket_fd_left == -1) {
        fprintf(stderr, "Error: cannot create socket for left player\n");
        fprintf(stderr, "  (%s, %s)", hostname_left, portNumber_left);
        return -1;
    }
    //printf("Connecting to the left player %s, on portNumber %s ...\n", hostname_left, portNumber_left);
    
    status_left = connect(socket_fd_left, host_info_list_left->ai_addr, host_info_list_left->ai_addrlen);

    if (status_left == -1) {
        printf("Error: cannot connect to the left player socket\n");
        return -1;
    }

    char playerConfirm[512];
    if(playerId != 0){
        sprintf(playerConfirm, "Player %d is connected with player %d\n", playerId, playerId - 1);
    }else{
        sprintf(playerConfirm, "Player %d is connected with player %d\n", playerId, playerNum - 1);
    }
    //send(socket_fd_left, playerConfirm, strlen(playerConfirm), 0);
    freeaddrinfo(host_info_list_left);
    return socket_fd_left;

}


int main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(stderr, "The input does not satisfy the requirement.");
        exit(1);
    }
    //hostname and portnumber of read_fds
    const char * hostname = argv[1];
    const char * portNumber = argv[2];

    /*****    Connect with the ringread_fds    *****/
    int socket_fd = connect_to_read_fds(hostname, portNumber);
    int playerNum = 0;
    int playerId = 0;
    recv(socket_fd, &playerNum, sizeof(int), 0);
    recv(socket_fd, &playerId, sizeof(int), 0);
    printf("Connected as player %d out of %d total players\n", playerId, playerNum);

    /***   Get hostname and send to ringread_fds   ***/
    char my_hostname[512];
    if(gethostname(my_hostname, 512) == -1){
        fprintf(stderr, "Cannot get my hostname\n");
        return -1;
    }
    int hostname_len = strlen(my_hostname);
    my_hostname[hostname_len] = 0;
    send(socket_fd, &hostname_len, sizeof(int), 0);
    int sendToRingread_fds = send(socket_fd, my_hostname, hostname_len, 0);
    if(sendToRingread_fds == -1){
        fprintf(stderr, "Cannot send my hostname to ringread_fds\n");
        return -1;
    }

    /** Set up the socket so that you can be the server and send to port to read_fds **/
    int socket_fd_right = setup_socket_for_right(playerId, playerNum);//, portNumber);
    int port_right = get_port_number(socket_fd_right);
    //send the port numebr of socket for right player to ringread_fds
    send(socket_fd, &port_right, sizeof(int), 0);
    

    /*** Received the message from the ringread_fds that all players are initilized. Start to link
     * them into a ring ***/
    char player_init_success_message[512];
    recv(socket_fd, player_init_success_message, 13, 0);
    player_init_success_message[13] = 0;

    char hostname_left[512];
    int rev = recv(socket_fd, &hostname_left, hostname_len, 0);
    hostname_left[hostname_len] = 0;

    /**** Connect with the player to the right by receiving their message *****/

    if(playerId == playerNum - 1) {
            int confirmReady = 2;
            send(socket_fd, &confirmReady, sizeof(int), 0);
    }
    /**Socket receiving from right **/
    int player_right_fd;
    int player_left_fd;

    int port_left = 0;

    /**** Connect with the player to the left by sending connect request like a client*****/
    if(playerId != 0){
        int test = recv(socket_fd, &port_left, sizeof(int), 0);
        if(test == -1){
            fprintf(stderr, "Cannot get the port number of left socket from ringread_fds\n");
            return -1;
        }
        player_left_fd = connect_to_left(playerId, playerNum, port_left, hostname_left);
        player_right_fd = accept_connect_from_right(socket_fd_right, playerId, playerNum);
    }    
    /**Player 0 will connect with the left player after connecting to the right player and receiving 
     * the message from ringread_fds that n-1 player is setting up the socket**/
    else{
        player_right_fd = accept_connect_from_right(socket_fd_right, playerId, playerNum);
        int confirm = -1;
        recv(socket_fd, &confirm, sizeof(int), 0);
        assert(confirm == 2);
        int test = recv(socket_fd, &port_left, sizeof(int), 0);
        if(test == -1){
            fprintf(stderr, "Cannot get the port number of left socket from ringread_fds\n");
            return -1;
        }
        player_left_fd = connect_to_left(playerId, playerNum, port_left, hostname_left);
    }

    // char play_right_confirm[512];
    // int r = recv(player_right_fd, play_right_confirm, 37, 0);
    // if(r == -1){
    //     printf("Did not connect\n");
    // }else{
    //     play_right_confirm[36] = 0;
    //     printf("%s", play_right_confirm);
    // }

    // char play_left_confirm[512];
    // r = recv(player_left_fd, play_left_confirm, 37, 0);
    // if(r == -1){
    //     printf("Did not connect\n");
    // }else{
    //     play_left_confirm[36] = 0;
    //     printf("%s\n", play_left_confirm);
    // }

    send(socket_fd, &playerId, sizeof(int), 0);

    Potato potato;
    int fdmax = 0;
    fd_set read_fds;  // temp file 
    srand((unsigned int)time(NULL)+ playerId);

    while(1){    
        FD_ZERO(&read_fds);    // clear
        FD_SET(socket_fd, &read_fds);
        if(socket_fd>fdmax){
            fdmax = socket_fd;
        }
        FD_SET(player_left_fd, &read_fds);
        if(player_left_fd>fdmax){
            fdmax = player_left_fd;
        }
        FD_SET(player_right_fd, &read_fds);
        if(player_right_fd>fdmax){
            fdmax = player_right_fd;
        }
        int test = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if(test == -1){
            printf("Error Select!\n");
            return -1;
        }
        int nbytes;
        for(int i = 0; i <= fdmax; i++){
            if(FD_ISSET(i, &read_fds)){
                nbytes = recv(i, &potato, sizeof(potato), 0);
                break;
            }
        }
        
        if(nbytes == 0){
            //printf("Did not receive any message, should end the game");
            break;
        }
        else if(potato.remainHops == 0){
            fprintf(stderr, "get the potato is wrong");
            break;
        }
        else if(potato.remainHops == 1){
            potato.remainHops = 0;
            potato.holderNum = playerId;
            potato.trace[potato.current_round] = playerId;
            potato.current_round +=1;
            send(socket_fd, &potato, sizeof(potato), 0);
            printf("I'm it\n");
        }
        else{
            //printf("current hops: %d\n", potato.remainHops);
            potato.remainHops -= 1;
            potato.trace[potato.current_round] = playerId;
            //printf("current trace: %d, %d\n", potato.current_round, potato.trace[potato.current_round]);
            potato.current_round += 1;
            int send_rand = rand()%2;
            if(send_rand == 0){
                send(player_left_fd, &potato, sizeof(potato), 0);
                if(playerId == 0){
                    printf("Sending potato to %d\n", playerNum - 1);
                }else{
                    printf("Sending potato to %d\n", playerId - 1);
                }
            }else{
                send(player_right_fd, &potato, sizeof(potato), 0);
                if(playerId == playerNum - 1){
                    printf("Sending potato to %d\n", 0);
                }else{
                    printf("Sending potato to %d\n", playerId + 1);
                }
            }
        }
    }
    
    close(socket_fd_right);
    close(player_left_fd);
    close(player_right_fd);
    close(socket_fd);
    return 0;
}
