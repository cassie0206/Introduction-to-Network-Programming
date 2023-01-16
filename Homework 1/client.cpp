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

int socketfdTCP, socketfdUDP;
bool isGame = false;
struct sockaddr_in serverInfo;
string input;

void UDP_send()
{ // UDP
    char sendBuff[1024] = {0}, recvBuff[1024] = {0};
    strcpy(sendBuff, input.c_str());
    int len = sizeof(serverInfo);

    sendto(socketfdUDP, sendBuff, sizeof(sendBuff), MSG_CONFIRM, (struct sockaddr *)&serverInfo, len);
    recvfrom(socketfdUDP, recvBuff, sizeof(recvBuff), MSG_WAITALL, (struct sockaddr *)&serverInfo, (socklen_t *)&len);

    printf("%s", recvBuff);
}

void TCP_send()
{
    char sendBuff[1024] = {0}, recvBuff[1024] = {0};
    strcpy(sendBuff, input.c_str());
    send(socketfdTCP, sendBuff, sizeof(sendBuff), 0);
    recv(socketfdTCP, recvBuff, sizeof(recvBuff), 0);
    printf("%s", recvBuff);

    // for start-game
    if (strcmp(recvBuff, "Please typing a 4-digit number:\n") == 0)
    {
        isGame = true;
    }
    else if (strcmp(recvBuff, "You got the answer!\n") == 0 || strcmp(recvBuff, "You lose the game!\n") == 0 || strcmp(recvBuff, "Your guess should be a 4-digit number\n") == 0)
    {
        isGame = false;
    }
}

int main(int argc, char *argv[])
{

    // UDP
    socketfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfdUDP == -1)
    {
        cout << "Oops! UDP socket creation failed!\n";
        exit(0);
    }

    // TCP
    socketfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfdTCP == -1)
    {
        cout << "Oops! TCP socket creation failed!\n";
        exit(0);
    }

    // server info
    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    // cout<<"port:"<<serverInfo.sin_port<<endl;

    // TCP connection
    if (connect(socketfdTCP, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1)
    {
        cout << "Oops! connection failed!\n";
        printf("can't connect to %s.%s: %s\n", argv[1], argv[2], hstrerror(errno));
        exit(0);
    }

    cout << "*****Welcome to Game 1A2B*****\n";

    while (1)
    {
        getline(cin, input);
        stringstream ss(input);
        string token;
        getline(ss, token, ' ');

        if (isGame)
        { // TCP strat-game
            TCP_send();
        }
        else if (token == "register")
        { // UDP register
            UDP_send();
        }
        else if (token == "login")
        { // TCP login
            TCP_send();
        }
        else if (token == "logout")
        { // TCP logout
            TCP_send();
        }
        else if (token == "game-rule")
        { // UDP game-rule
            UDP_send();
        }
        else if (token == "start-game")
        { // TCP strat-game
            TCP_send();
        }
        else if (token == "exit")
        { // TCP exit
            TCP_send();
            break;
        }
        else
        {
            cout << "command not found!\n";
            exit(0);
        }
    }

    close(socketfdTCP);
    close(socketfdUDP);

    return 0;
}