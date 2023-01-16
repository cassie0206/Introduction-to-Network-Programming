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
#include <queue>
#include <arpa/inet.h>

using namespace std;

vector<int> userData; // [clientSocketfd] = #user

vector<string> split(char* str){
    stringstream ss(str);
    string token;
    vector<string> v;
    while(getline(ss, token, ' ')){
        v.push_back(token);
    }

    return v;
}

int main(int argc, char *argv[]){
    int socketfd, max_client = 20;
    struct sockaddr_in serverInfo, clientInfo;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1){
        cout<<"Oops! TCP socket connection failed!\n";
        exit(0);
    }

    int optimal = 1;
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optimal,sizeof(int)) == -1){
        cout<<"Oops! socket setup failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[1]));
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(socketfd, (struct sockaddr*) &serverInfo,sizeof(serverInfo)) == -1){
        cout<<"Oops! socket bind failed!\n";
        exit(0);
    }

    if(listen(socketfd, max_client) == -1){
        cout <<"Oops! socket listen failed!\n";
        exit(0);
    }

    fd_set rfds;
    vector<int> clientSocketfd(max_client, 0);
    vector<int> userData(max_client, -1);
    bzero(&clientInfo, sizeof(clientInfo));
    int len =sizeof(clientInfo);
    int serverlen =sizeof(serverInfo);
    int num = 1;

    while(1){
        FD_ZERO(&rfds);
        FD_SET(socketfd, &rfds);
        int maxfd = socketfd;

        for(int i=0;i<max_client;i++){
            if(clientSocketfd[i] > 0){
                FD_SET(clientSocketfd[i], &rfds);
                if(maxfd < clientSocketfd[i]){
                    maxfd = clientSocketfd[i];
                }
            }
        }

        int n =select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if(n == -1){
            cout<<"Oops! select failed!\n";
            exit(0);
        }

        if(FD_ISSET(socketfd, &rfds)){
            int newSocket = accept(socketfd, (struct sockaddr*) &serverInfo, (socklen_t*) &serverlen);
            if(newSocket == -1){
                cout<<"Oops! socket accept failed!\n";
                exit(0);
            }

            cout<<"New connection from "<<inet_ntoa(serverInfo.sin_addr)<<":"<<ntohs(serverInfo.sin_port)<<" user"<<num<<"\n";
            //printf("New connection from %s:%d %s\n", inet_ntoa(serverInfo.sin_addr), ntohs(serverInfo.sin_port), "user" + to_string(num));
            char sendBuff[10000] = {0};
            string str = "Welcome, you are user" + to_string(num) + "\n";
            strcpy(sendBuff, str.c_str());
            send(newSocket, sendBuff, sizeof(sendBuff), 0);

            for(int i=0;i<max_client;i++){
                if(clientSocketfd[i] == 0){
                    clientSocketfd[i] = newSocket;
                    userData[i] = num++;
                    break;
                }
            }
            
        }

        for(int i=0;i<max_client;i++){
            char recvBuff[10000] = {0}, sendBuff[10000] = {0};
            if(FD_ISSET(clientSocketfd[i], &rfds)){
                if(recv(clientSocketfd[i], recvBuff, sizeof(recvBuff), 0) == 0){
                    getpeername(clientSocketfd[i] , (struct sockaddr*)&serverInfo , (socklen_t*)& serverlen);  
                    printf("user%d %s:%d disconnected\n" , userData[i], inet_ntoa(serverInfo.sin_addr) , ntohs(serverInfo.sin_port));  
                         
                    close(clientSocketfd[i]);  
                    clientSocketfd[i] = 0;  
                    userData[i] = -1;
                }
                else{
                    //cout<<"TCP get msg: "<<recvBuff<<endl;
                    vector<string> v =split(recvBuff);
                    string output;
                    if(v[0] == "list-users"){
                        priority_queue<int, vector<int>, greater<int>> q;
                        for(int j=0;j<max_client;j++){
                            if(userData[j] != -1){
                                q.push(userData[j]);
                            }
                        }
                        while(!q.empty()){
                            output += "user" + to_string(q.top()) + "\n";
                            q.pop();
                        }
                    }
                    else if(v[0] == "get-ip"){
                        getpeername(clientSocketfd[i] , (struct sockaddr*)&serverInfo , (socklen_t*)& serverlen);
                        output += "IP: " + string(inet_ntoa(serverInfo.sin_addr)) + ":" + to_string(ntohs(serverInfo.sin_port)) + "\n";
                    }
                    else if(v[0] == "exit"){
                        getpeername(clientSocketfd[i] , (struct sockaddr*)&serverInfo , (socklen_t*)& serverlen);  
                        printf("user%d %s:%d disconnected\n" , userData[i], inet_ntoa(serverInfo.sin_addr) , ntohs(serverInfo.sin_port));  
                        
                        output += "Bye user" + to_string(userData[i]) + ".\n";
                        userData[i] = -1;
                        strcpy(sendBuff, output.c_str());
                        send(clientSocketfd[i], sendBuff, sizeof(sendBuff), 0);
                        close(clientSocketfd[i]);
                        clientSocketfd[i] = 0;
                    }
                    strcpy(sendBuff, output.c_str());
                    send(clientSocketfd[i], sendBuff, sizeof(sendBuff), 0);
                }
            }
        }



    }



    return 0;
}