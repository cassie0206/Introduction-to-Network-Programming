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

int socketfdTCP;
struct sockaddr_in serverInfo;
string input;

void TCP_send(){
    char sendBuff[10024] = {0}, recvBuff[1024] = {0};
    send(socketfdTCP, sendBuff, sizeof(sendBuff), 0);
}


int main(int argc, char *argv[])
{
    socketfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfdTCP == -1){
        cout<<"Oops! TCP socket creation failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(socketfdTCP, (struct sockaddr*)& serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! connection failed!\n";
        printf("can't connect to %s.%s: %s\n", argv[1], argv[2], hstrerror(errno));
        exit(0);
    }

    cout << "**************************\n*Welcome to the BBS server.*\n**************************\n";

    fd_set rfds;
    int stdfd = fileno(stdin);
    int maxfd = max(socketfdTCP, stdfd);
    FD_ZERO(&rfds);
    
    while(1){
        FD_SET(stdfd, &rfds);
        FD_SET(socketfdTCP, &rfds);

        select(maxfd + 1, &rfds, NULL, NULL, NULL);

        // command
        if(FD_ISSET(stdfd, &rfds)){
            char recvBuff[1024] = {0};
            int n = read(stdfd, recvBuff, sizeof(recvBuff));
            stringstream ss(recvBuff);
            string token;
            getline(ss, token, ' ');
         
            if(token == "mute\n" || token == "unmute\n" || token == "yell" || token == "tell" || token == "exit\n" || token == "mute" || token == "unmute" || token == "exit"){
                send(socketfdTCP, recvBuff, sizeof(recvBuff), 0);
            }
            else{
                cout<<"comman not found!\ncommand list:\n  mute\n  unmute\n  yell <message>\n  tell <receiver> <message>\n  exit\n";
            }
        }
        // server reply
        if(FD_ISSET(socketfdTCP, &rfds)){
            char recvBuff[1024] = {0};
            if(recv(socketfdTCP, recvBuff, sizeof(recvBuff), 0) == 0){
                break;      
            }
            printf("%s", recvBuff);
        }
        
    }

    close(socketfdTCP);

    return 0;
}