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
#include <dirent.h>
#include <fstream>
//#include <sys/type.h>

using namespace std;
string input;
int socketfd;
struct sockaddr_in serverInfo;


void UDP_send(){
    char sendBuff[1024] = {0}, recvBuff[1024] = {0};
    strcpy(sendBuff, input.c_str());
    int len = sizeof(serverInfo);

    sendto(socketfd, sendBuff, sizeof(sendBuff), MSG_CONFIRM, (struct sockaddr*) &serverInfo, len);
    recvfrom(socketfd, recvBuff, sizeof(recvBuff), MSG_WAITALL, (struct sockaddr*) &serverInfo, (socklen_t*) &len);

    printf("%s", recvBuff);
}

void UDP_getfile(){
    char sendBuff[1024] = {0}, recvBuff[10000] = {0};
    strcpy(sendBuff, input.c_str());
    int len = sizeof(serverInfo);

    sendto(socketfd, sendBuff, sizeof(sendBuff), MSG_CONFIRM, (struct sockaddr*) &serverInfo, len);

    stringstream ss(input);
    string token;   
    while(getline(ss, token, ' ')){
        if(token == "get-file"){
            continue;
        }
        ofstream f;
        f.open(token);
        recvfrom(socketfd, recvBuff, sizeof(recvBuff), MSG_WAITALL, (struct sockaddr*) &serverInfo, (socklen_t *) &len);
        f << string(recvBuff);

        f.close();
    }
}



int main(int argc, char *argv[]){
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfd == -1){
        cout << "Oops! socket failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);

    while(1){
        cout<<"% ";
        getline(cin, input);
        stringstream ss(input);
        string token;
        getline(ss, token, ' ');
        
        if(token == "get-file-list"){
            UDP_send();
        }
        else if(token == "get-file"){
            UDP_getfile();
        }
        else if(token == "exit"){
            UDP_send();
            break;
        }
        else{
            cout<< "command not found!\n";
            continue;
        }
    }

    close(socketfd);


    return 0;
}