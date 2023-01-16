#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <sys/select.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string.h>
#include <map>

using namespace std;

struct USERS{
    bool isMute = false;
    int index = -1; // user index
    int pos = -1; // clientsocketfd index
};

vector<string> split(char* str){
    stringstream ss(str);
    string token;
    vector<string> v;
    
    while(getline(ss, token, ' ')){
        v.push_back(token);
    }

    return v;
}

int main(int argc, char* argv[])
{
    int socketfdTCP, max_client = 20, count = 0;
    struct sockaddr_in serverInfo, clientInfo;
    
    socketfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfdTCP == -1){
        cout<<"Oops! socket creation failed!\n";
        exit(0);
    }

    int optimal = 1;
    if(setsockopt(socketfdTCP, SOL_SOCKET, SO_REUSEADDR, &optimal, sizeof(int)) == -1){
        cout<<"Oops! socket setup failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[1]));
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);

    //TCP bind
    if(bind(socketfdTCP, (struct sockaddr*)& serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! socket bind failed!\n";
        exit(0);
    }

    if(listen(socketfdTCP, max_client) == -1){
        cout<<"Oops! socket listen failed!\n";
        exit(0);
    }
    //cout<<"TCP server is running\n";

    fd_set rfds;
    vector<int> clientsocketfd(max_client, 0);
    bzero(&clientInfo, sizeof(clientInfo));
    vector<USERS> userData;

    while(1){
        FD_ZERO(&rfds);
        FD_SET(socketfdTCP, &rfds);
        int maxfd = socketfdTCP;

        for(int i=0;i<max_client;i++){
            FD_SET(clientsocketfd[i], &rfds);
            maxfd = max(maxfd, clientsocketfd[i]);
        }

        int n = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if(n == -1){
            cout<<"Oops! select failed!\n";
            exit(0);
        }

        // new connection
        if(FD_ISSET(socketfdTCP, &rfds)){
            int buffSize;
            int serverlen = sizeof(serverInfo);
            int newSocket = accept(socketfdTCP, (struct sockaddr*)& serverInfo, (socklen_t*)& serverlen);
            if(newSocket == -1){
                cout<<"Oops! socket accept failed!\n";
                exit(0);
            }
            //cout<<"New connection.\n";
            int keep = -1;
            for(int i=0;i<max_client;i++){
                if(clientsocketfd[i] == 0){
                    clientsocketfd[i] = newSocket;
                    keep = i;
                    break;
                }
            }

            USERS user;
            user.index = count++;
            user.pos = keep;
            userData.push_back(user);

            char sendBuff[1024] = {0};
            string str = "Welcome, user" + to_string(user.index) + ".\n";
            buffSize = str.size();
            strcpy(sendBuff, str.c_str());
            send(newSocket, sendBuff, buffSize, 0);
        }

        for(int i=0;i<max_client;i++){
            char recvBuff[1024] = {0}, sendBuff[1024] = {0};
            bool isSend = false;
            int buffSize;
            if(FD_ISSET(clientsocketfd[i], &rfds)){
                if(recv(clientsocketfd[i], recvBuff, sizeof(recvBuff), 0) == 0){

                }
                else{
                     //cout<<"TCP get msg:"<<recvBuff<<endl;
                     vector<string> v = split(recvBuff);
                     if(v[0] == "mute\n"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                if(userData[j].isMute){
                                    string str = "You are already in mute mode.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    userData[j].isMute = true;
                                    string str = "Mute mode.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                            }
                        }
                     }
                     else if(v[0] == "unmute\n"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                if(userData[j].isMute){
                                    userData[j].isMute = false;
                                    string str = "Unmute mode.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    string str = "You are already in unmute mode.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                            }
                        }
                     }
                     else if(v[0] == "yell"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                string str = "user" + to_string(userData[j].index) + ":";
                                for(int k=1;k<v.size();k++){
                                    str += " " + v[k];
                                }
                                buffSize = str.size();
                                strcpy(sendBuff, str.c_str());
                            
                                for(int k=0;k<userData.size();k++){
                                    if(k != j && !userData[k].isMute && userData[k].pos != -1){
                                        cout<<"user"<<userData[k].index<<endl;
                                        send(clientsocketfd[userData[k].pos], sendBuff, buffSize, 0);
                                    }
                                }
                            }
                        }
                        isSend = true;
                     }
                    else if(v[0] == "tell"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                int recv_pos = -1;
                                bool isrecvMute;
                                for(int k=0;k<userData.size();k++){
                                    if(userData[k].index == v[1][4] - '0' && v[1][0] == 'u' && v[1][1] == 's' && v[1][2] == 'e' && v[1][3] == 'r'){
                                        recv_pos = userData[k].pos;
                                        isrecvMute = userData[k].isMute;
                                        break;
                                    }
                                }

                                if(recv_pos == -1){
                                    string str = v[1] + " does not exist.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                    break;
                                }
                                else if(recv_pos == i){
                                    string str = "client cannot send a message to himself.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                    break;
                                }
                                else if(isrecvMute){
                                    string str = v[1] + " is in mute mode.\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                    break;
                                }
                                else if(!isrecvMute){
                                    string str = "user" + to_string(userData[j].index) + " told you:";
                                    for(int k=2;k<v.size();k++){
                                        str += " " + v[k];
                                    }
                                    if(v.size() == 2){
                                        str += '\n';
                                    }
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                    send(clientsocketfd[recv_pos], sendBuff, buffSize, 0);
                                    isSend = true;
                                }
                            }
                        }
                    }
                    else if(v[0] == "exit\n"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                userData[j].pos = -1;
                            }
                        }
                        close(clientsocketfd[i]);
                        clientsocketfd[i] = 0;
                        strcpy(sendBuff, " ");
                    }
                    
                    if(isSend){
                        continue;
                    }
                    send(clientsocketfd[i], sendBuff, buffSize, 0);
                }
            }
        }
        
    }


    return 0;
}