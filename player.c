#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "potato.h"

int connect_to_master(const char* hostname, const char *portNumber){
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

int setup_socket_for_right(int playerId, int playerNum, const char* portNumber){
    int status_server;
    int socket_fd_right;
    struct addrinfo host_info_server;
    struct addrinfo *host_info_list_server;

    memset(&host_info_server, 0, sizeof(host_info_server));
    host_info_server.ai_family   = AF_UNSPEC;
    host_info_server.ai_socktype = SOCK_STREAM;
    host_info_server.ai_flags = AI_PASSIVE;
    char * hostname_server = NULL;
    int port_right = atoi(portNumber) + playerId + 1;
    char portNumber_right[strlen(portNumber)];
    sprintf(portNumber_right, "%d", port_right);

    status_server = getaddrinfo(hostname_server, portNumber_right, &host_info_server, &host_info_list_server);
    
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
    send(player_right_fd, confirm, strlen(confirm), 0);
    return player_right_fd;
}

/**** Connect with the player to the left by sending connect request like a client*****/
int connect_to_left(int playerId, int playerNum, const char *portNumber, const char * hostname_left){
    int status_left;
    int socket_fd_left;
    struct addrinfo host_info_left;
    struct addrinfo *host_info_list_left;

    memset(&host_info_left, 0, sizeof(host_info_left));
    host_info_left.ai_family   = AF_UNSPEC;
    host_info_left.ai_socktype = SOCK_STREAM;
    host_info_left.ai_flags = AI_PASSIVE;

    int port_left = 0;
    if(playerId != 0){
        port_left = atoi(portNumber) + playerId;
    }else{
        port_left = atoi(portNumber) + playerNum;
    }
    char portNumber_left[strlen(portNumber)];
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
    printf("Connecting to the left player %s, on portNumber %s ...\n", hostname_left, portNumber_left);
    
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
    send(socket_fd_left, playerConfirm, strlen(playerConfirm), 0);
    return socket_fd_left;

}

int main(int argc, char *argv[])
{
    assert(argc == 3);
    //hostname and portnumber of master
    const char * hostname = argv[1];
    const char * portNumber = argv[2];

    /*****    Connect with the ringmaster *****/
    int socket_fd = connect_to_master(hostname, portNumber);
    int playerNum = 0;
    int playerId = 0;
    recv(socket_fd, &playerNum, sizeof(int), 0);
    recv(socket_fd, &playerId, sizeof(int), 0);

    /***Get hostname and send to ringmaster **/
    char my_hostname[512];
    if(gethostname(my_hostname, 512) == -1){
        fprintf(stderr, "Cannot get my hostname\n");
        return -1;
    }
    int hostname_len = strlen(my_hostname);
    my_hostname[hostname_len] = 0;
    send(socket_fd, &hostname_len, sizeof(int), 0);
    int sendToRingmaster = send(socket_fd, my_hostname, hostname_len, 0);
    if(sendToRingmaster == -1){
        fprintf(stderr, "Cannot send my hostname to ringmaster\n");
        return -1;
    }

    /*** Received the message from the ringmaster that all players are initilized. Start to link
     * them in to a ring ***/
    char player_init_success_message[512];
    recv(socket_fd, player_init_success_message, 13, 0);
    player_init_success_message[13] = 0;
    printf("%s\n", player_init_success_message);

    char hostname_left[512];
    int rev = recv(socket_fd, &hostname_left, hostname_len, 0);
    hostname_left[hostname_len] = 0;
    printf("Receive left player hostname: %s\n", hostname_left);
    printf("recv_len = %d\n", rev);

    char hostname_right[512];
    rev = recv(socket_fd, &hostname_right, hostname_len, 0);
    hostname_right[hostname_len] = 0;
    printf("Receive right player hostname: %s\n", hostname_right);
    printf("recv_len = %d\n", rev);

    /**** Connect with the player to the right by receiving their message *****/
    /** First, set up the socket so that you can be the server **/
    int socket_fd_right = setup_socket_for_right(playerId, playerNum, portNumber);
    if(playerId == playerNum - 1) {
            int confirmReady = 2;
            send(socket_fd, &confirmReady, sizeof(int), 0);
    }
    /**Socket receiving from right **/
    int player_right_fd;
    int player_left_fd;

    /**** Connect with the player to the left by sending connect request like a client*****/
    if(playerId != 0){
        player_left_fd = connect_to_left(playerId, playerNum, portNumber, hostname_left);
        player_right_fd = accept_connect_from_right(socket_fd_right, playerId, playerNum);
    }    
    /**Player 0 will connect with the left player after connecting to the right player and receiving 
     * the message from ringmaster that n-1 player is setting up the socket**/
    else{
        player_right_fd = accept_connect_from_right(socket_fd_right, playerId, playerNum);
        int confirm = -1;
        recv(socket_fd, &confirm, sizeof(int), 0);
        assert(confirm == 2);
        player_left_fd = connect_to_left(playerId, playerNum, portNumber, hostname_left);
    }

    char play_right_confirm[512];
    int r = recv(player_right_fd, play_right_confirm, 36, 0);
    if(r == -1){
        printf("Did not connect\n");
    }else{
        play_right_confirm[36] = 0;
        printf("%s", play_right_confirm);
    }

    char play_left_confirm[512];
    r = recv(player_left_fd, play_left_confirm, 36, 0);
    if(r == -1){
        printf("Did not connect\n");
    }else{
        play_left_confirm[36] = 0;
        printf("%s\n", play_left_confirm);
    }

    send(socket_fd, &playerId, sizeof(int), 0);


    /*** Start the game***/
    close(socket_fd);

    return 0;
}
