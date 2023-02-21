#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include "potato.h"

int main (int argc, char * argv[]){
    /* Check the input argument
     *   playerNumber > 1 : how many gamer in the current game
     *  0 <= hopNumber <= 512 : how many round of giving potato will present
     */
    assert(argc == 4);
    const char * portNumber = argv[1];
    const char * playerNumber = argv[2];
    const char * hopsNumber = argv[3];
    int playerNum = atoi(playerNumber);
    int hopsNum = atoi(hopsNumber);
    assert(playerNum > 1);
    assert(hopsNum >= 0 && hopsNum <= 512);

    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));

    const char *hostname = NULL;
    
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, portNumber, &host_info, &host_info_list);
    if (status != 0) {
        fprintf(stderr, "Error: cannot get address info for host\n");
        fprintf(stderr, "  ( %s, %s)\n", hostname, portNumber);
        return -1;
    }

    socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
    if (socket_fd == -1) {
        fprintf(stderr, "Error: cannot create socket\n");
        fprintf(stderr, "  ( %s, %s)\n", hostname, portNumber);
        return -1;
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        fprintf(stderr, "Error: cannot bind socket\n");
        printf("  ( %s, %s)\n", hostname, portNumber);
    }

    status = listen(socket_fd, 100);
    if (status == -1) {
        fprintf(stderr, "Error: cannot listen on socket"); 
        fprintf(stderr, "  ( %s, %s)\n", hostname, portNumber);
        return -1;
    }

    //Successfully init the socket, print game init information
    //TODO: Change output after tuning
    printf("Potato Ringmaster, port number: %s\n", portNumber);
    printf("Players = <%s>\n", playerNumber);
    printf("Hops = <%s>\n", hopsNumber);

    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);

    //Initialize the players and connect them with ring master
    int player_connection_fd[playerNum];
    char player_hostname[playerNum][512];
    
    for(int i = 0; i < playerNum; i++){
        player_connection_fd[i] = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (player_connection_fd[i] == -1) {
            fprintf(stderr, "Error: cannot accept connection on socket.\n");
            return -1;
        }
        int playerId = i;
        send(player_connection_fd[i], &playerNum, sizeof(int), 0);
        send(player_connection_fd[i], &playerId, sizeof(int), 0);
        int player_hostname_len = 0;
        recv(player_connection_fd[i], &player_hostname_len, sizeof(int), 0);
        recv(player_connection_fd[i], &player_hostname[i], player_hostname_len, 0);
        player_hostname[i][player_hostname_len] = 0;
    }

    for(int i = 0; i < playerNum; i++){
        const char * message = "now make ring";
        send(player_connection_fd[i], message, strlen(message), 0);
        //send left player's hostname 
        int left_player_id = (playerNum + i - 1) % playerNum;
        send(player_connection_fd[i], &player_hostname[left_player_id], strlen(player_hostname[left_player_id]), 0);
        //send right player's hostname
        int right_player_id = (playerNum + i + 1) % playerNum;
        send(player_connection_fd[i], &player_hostname[right_player_id], strlen(player_hostname[right_player_id]), 0);
        //Confirm that the socket for player 0 on player n-1 is ready
        if(i == playerNum - 1){
            int confirmReady = 0;
            recv(player_connection_fd[i], &confirmReady, sizeof(int), 0);
            printf("Received the confirmation from n - 1, player 0 can send request to n - 1\n");
            assert(confirmReady == 2);
            send(player_connection_fd[0], &confirmReady, sizeof(int), 0);
        }
    }
    //receive ready message from every player
    for(int i = 0; i < playerNum; i++){
        int recPlayerId = 0;
        int test = recv(player_connection_fd[i], &recPlayerId, sizeof(int), 0);
        if(test < 0){
            printf("Error: player %d is not ready", recPlayerId);
        }
        printf("Player <%d> is ready to play.\n", recPlayerId);
    }
    

    //Initialize the potato
    Potato ourPotato;
    ourPotato.remainHops = hopsNum;
    ourPotato.current_round = 0;
    Potato returnPotato;

    if(hopsNum > 0){
        srand((unsigned int)time(NULL)+playerNum);
        int holder = rand() % playerNum;
        printf("Ready to start the game, sending potato to player <%d>\n", holder);
        send(player_connection_fd[holder], &ourPotato, sizeof(ourPotato), 0);

        //receive the potato when game is over
        fd_set player_fds;
        int fdmax = 0;
        FD_ZERO(&player_fds);
        for(int i = 0; i < playerNum; i++){
            FD_SET(player_connection_fd[i], &player_fds);
            if(player_connection_fd[i] > fdmax){
                fdmax = player_connection_fd[i];
            }
        }
        int test = select(fdmax+1, &player_fds, NULL, NULL, NULL);
        if(test == -1){
            printf("Error Select!\n");
            return -1;
        }
        for (int i = 0; i < playerNum; i++) {
            if (FD_ISSET(player_connection_fd[i], &player_fds)) {
                recv(player_connection_fd[i], &returnPotato, sizeof(returnPotato), 0);
                if(returnPotato.remainHops != 0){
                    printf("Error: The potato is passed back to ringmaster when the game is not over");
                    return -1;
                }
                if(returnPotato.holderNum != i){
                    printf("Error: The potato did not come from the right player");
                    return -1;
                }
                break;
            }
        }
    }
    //remainshopsNum == 0, close the game
    for(int i = 0; i < playerNum; i++){
        send(player_connection_fd[i], &returnPotato, sizeof(ourPotato), 0);
    }
    if(hopsNum != 0){
        //print the trace
        printf("Trace of potato: \n");
        for(int j = 0; j < hopsNum - 1; j++){
            printf("%d, ", returnPotato.trace[j]);
        }
        printf("%d\n ", returnPotato.trace[hopsNum - 1]);
    }
    for(int i = 0; i < playerNum; i++){
        close(player_connection_fd[i]);
    }
    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
}
