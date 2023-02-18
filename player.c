#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "potato.h"

int main(int argc, char *argv[])
{
    assert(argc == 3);
    //hostname and portnumber of master
    const char * hostname = argv[1];
    const char * portNumber = argv[2];

    /*****    Connect with the ringmaster *****/
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
        fprintf(stderr, "(%s, %s)", hostname, portNumber);
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
    int playerNum = 0;
    int playerId = 0;
    recv(socket_fd, &playerNum, sizeof(int), 0);
    recv(socket_fd, &playerId, sizeof(int), 0);
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
    /** First, set up the socket for right player and listen **/
    int status_right;
    int socket_fd_right;
    struct addrinfo host_info_right;
    struct addrinfo *host_info_list_right;

    memset(&host_info_right, 0, sizeof(host_info_right));
    host_info_right.ai_family   = AF_UNSPEC;
    host_info_right.ai_socktype = SOCK_STREAM;
    host_info_right.ai_flags = AI_PASSIVE;

    int port_right = atoi(portNumber) + playerId + 1;

    char portNumber_right[strlen(portNumber)];
    sprintf(portNumber_right, "%d", port_right);

    status_right = getaddrinfo(hostname_right, portNumber_right, &host_info_right, &host_info_list_right);
    
    if (status_right != 0) {
        fprintf(stderr, "Error: cannot get address info for right player\n");
        return -1;
    }

    socket_fd_right = socket(host_info_list_right->ai_family, 
                host_info_list_right->ai_socktype, 
                host_info_list_right->ai_protocol);

    if (socket_fd_right == -1) {
        fprintf(stderr, "Error: cannot create socket for right player\n");
        return -1;
    }
    printf("Bind the socket for the right player %s, on portNumber %s ...\n", hostname_right, portNumber_right);
    
    int yes = 1;
    status_right = setsockopt(socket_fd_right, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status_right = bind(socket_fd_right, host_info_list_right->ai_addr, host_info_list_right->ai_addrlen);
    if (status == -1) {
        fprintf(stderr, "Error: cannot bind socket for right player\n");
        fprintf(stderr, "  (%s, %s)", hostname_right, portNumber_right);
        return -1;
    } 
    status_right = listen(socket_fd_right, 100);
    if (status == -1) {
        fprintf(stderr, "Error: cannot listen to socket for right player\n");
        fprintf(stderr, "  (%s, %s)", hostname_right, portNumber_right);
        return -1;
    }
    if(playerId == playerNum - 1) {
        int confirmReady = 2;
        send(socket_fd, &confirmReady, sizeof(int), 0);
    }

    /**** Connect with the player to the left by sending connect request like a client*****/
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

    /**Player 0 will connect with the left player after connecting to the right player and receiving 
     * the message from ringmaster that n-1 player is setting up the socket**/
    if(playerId != 0){
        status_left = getaddrinfo(hostname_left, portNumber_left, &host_info_left, &host_info_list_left);
        
        if (status_left != 0) {
            fprintf(stderr, "Error: cannot get address info for left player\n");
            fprintf(stderr, "(%s, %s)", hostname, portNumber);
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
        sprintf(playerConfirm, "Player %d is connected with player %d\n", playerId, playerId - 1);
        send(socket_fd_left, playerConfirm, strlen(playerConfirm), 0);

    }

    /** receive and accpet the connect from right player**/

    struct sockaddr_storage socket_addr_right;
    socklen_t socket_addr_len_right = sizeof(socket_addr_right);
    int player_right_fd = accept(socket_fd_right, (struct sockaddr *)&socket_addr_right, &socket_addr_len_right);
    if (player_right_fd == -1) {
        printf("Error: cannot connect to the right player socket\n");
        return -1;
    }else{
        printf("Accept the right player %s, on portNumber %s\n", hostname_right, portNumber_right);
    }

    
    /** Now, connect player 0 with its left player: player n-1 **/
    if(playerId == 0){
        int confirmReady = 0;
        recv(socket_fd, &confirmReady, sizeof(int), 0);
        assert(confirmReady == 2);

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
            return -1;
        }
        printf("Connecting to the left player %s, on portNumber %s ...\n", hostname_left, portNumber_left);
        
        status_left = connect(socket_fd_left, host_info_list_left->ai_addr, host_info_list_left->ai_addrlen);

        if (status_left == -1) {
        printf("Error: cannot connect to the left player socket\n");
        return -1;
        }else{
            printf("Connected to the left player %s, on portNumber %s\n", hostname_left, portNumber_left);
            char playerConfirm[512];
            sprintf(playerConfirm, "Player %d is connected with player %d\n", playerId, playerId + playerNum - 1);
            printf("%ld", strlen(playerConfirm));
            send(socket_fd_left, playerConfirm, strlen(playerConfirm), 0);
        }
        
    }

    char play_right_confirm[512];
    int test = recv(socket_fd_right, play_right_confirm, 36, 0);
    play_right_confirm[35] = 0;
    if(test == -1){
        printf("Did not connect\n");
    }else{
        printf("%s\n", play_right_confirm);
    }

    // char play_left_confirm[512];
    // test = recv(socket_fd_left, play_left_confirm, 35, 0);
    // play_left_confirm[35] = 0;
    // if(test == -1){
    //     printf("Did not connect to elft\n");
    // }
    // printf("%s\n", play_left_confirm);
    // if(playerId == 0){
    //     char playerConfirm[512];
    //     sprintf(playerConfirm, "Player %d is connected with player %d\n", playerId, playerId + 1);
    //     send(socket_fd_right, playerConfirm, strlen(playerConfirm), 0);
    // }
    // if(playerId == 1){
    //     char play_left_confirm[512];
    //     recv(socket_fd_left, play_left_confirm, 36, 0);
    //     play_left_confirm[36] = 0;
    //     printf("%s\n", play_left_confirm); 
    // }

    
    //The bi-directional connection between this player and the player on his right should be estbilised
    // if(playerId != playerNum - 1){
    //     char playerRightConfirm[512];
    //     sprintf(playerRightConfirm, "Player %d are connected with player %d", playerId, playerId + 1);
    //     send(socket_fd_right, playerRightConfirm, strlen(playerRightConfirm), 0);
    //     char play_left_confirm[512];
    //     recv(socket_fd_left, play_left_confirm, 36, 0);
    //     play_left_confirm[36] = 0;
    //     printf("%s\n", play_left_confirm);
    // }
/*****The bi-directional connection between this player and the player on his left shou be estblished*/

    freeaddrinfo(host_info_list);
    close(socket_fd);

    return 0;
}
