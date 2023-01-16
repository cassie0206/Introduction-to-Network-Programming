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
#include <filesystem>
#include <fstream>

using namespace std;

vector<string> split(char* str){
    stringstream ss(str);
    string token;
    vector<string> v;

    while(getline(ss, token, ' ')){
        v.push_back(token);
    }

    return v;
}


int main(int argc, char* argv[]){
    int socketfd;
    struct sockaddr_in serverInfo, clientInfo;

    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfd == -1){
        cout<<"Oops! socket failed!\n";
        exit(0);
    }

    int optimal = 1;
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optimal, sizeof(int)) == -1){
        cout<<"Oops! UDP socket setup failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[1]));
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(socketfd, (struct sockaddr*) &serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! UDP socket bind failed!\n";
        exit(0);
    }
    
    fd_set rfds;
    int len = sizeof(clientInfo);

    while(1){
        FD_ZERO(&rfds);
        FD_SET(socketfd, &rfds);

        if(FD_ISSET(socketfd, &rfds)){
            char recvBuff[1024] = {0}, sendBuff[10000] = {0};
            recvfrom(socketfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr*) &clientInfo, (socklen_t*) &len);
            //cout<<"UDP get msg:"<< recvBuff << endl;
            vector<string> v = split(recvBuff);
            string output;

            if(v[0] == "get-file-list"){
                output += "File: ";
                for (const auto & entry : std::filesystem::directory_iterator("./")){
                    string str = entry.path();
                    output += str.substr(2);
                    output += " ";
                }
                output += "\n";
                strcpy(sendBuff, output.c_str());
            }
            else if(v[0] == "get-file"){
                for(int i=1;i<v.size();i++){
                    ifstream f;
                    char buff[10000] = {0};

                    f.open(v[i]);
                    f.read(buff, sizeof(buff));
                    f.close();

                    sendto(socketfd, buff, sizeof(buff), 0, (struct sockaddr*) &clientInfo, len);
                }
                strcpy(sendBuff, "");
            }
            else if(v[0] == "exit"){
                strcpy(sendBuff, "");
            }

            sendto(socketfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr*) &clientInfo, len);
        }
    }
    
    
    
    return 0;
}