#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <string>
#include <string.h>
#include <strings.h> // bzero()
#include <unistd.h>  // read(), write(), close()
#include <sys/wait.h>
#include <sstream>
//#include <sys/type.h>

using namespace std;

int socketfd;
struct sockaddr_in serverInfo;
string input;

void TCP_send(){
    char sendBuff[10000] = {0}, recvBuff[10000] = {0};
    strcpy(sendBuff, input.c_str());
    send(socketfd, sendBuff, sizeof(sendBuff), 0);
    recv(socketfd, recvBuff, sizeof(recvBuff), 0);

    printf("%s", recvBuff);
}

int main(int argc, char *argv[]){
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1){
        cout << "Oops! TCP socket creation failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(socketfd, (struct sockaddr*) &serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! connection failed!\n";
        exit(0);
    }

    char recvBuff[10000] = {0};
    recv(socketfd, recvBuff, sizeof(recvBuff), 0);
    printf("%s", recvBuff);

    while(1){
        cout<<"% ";
        cin>>input;
        if(input == "list-users"){
            TCP_send();
        }
        else if(input == "get-ip"){
            TCP_send();
        }
        else if(input == "exit"){
            TCP_send();
            break;
        }
        else{
            cout<<"Command not found, please enter command again!\n";
            cout << "Command option:\nlist-users\nget-ip\nexit\n";
        }
    }

    close(socketfd);

    return 0;
}