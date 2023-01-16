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

typedef struct USERS{
    string name;
    string email;
    string password;
    int pos = -1;//keep login/logout
    string ans;//1A2B
    int time = 0;//for 1A2B timer
};

vector<string>split(char* str){
    stringstream ss(str);
    string token;
    vector<string> v;
    
    while(getline(ss, token, ' ')){
        v.push_back(token);
    }

    return v;
}

string game1A2B(string ans, string guess, int time){
    int a = 0, b = 0;
    map<char, int>ma, mg;
    string str;

    for(int i=0;i<4;i++){
        if(ans[i] == guess[i]){
            a++;
        }
        ma[ans[i]]++;
        mg[guess[i]]++;
    }

    for(auto it = ma.begin(); it != ma.end(); it++){
        auto iter = mg.find(it ->first);
        if(iter != mg.end()){
            b += min(it->second, iter->second);
        }
    }
    b -= a;

    if(a == 4 && b == 0){
        str = "You got the answer!\n";
    }
    else{
        if(time == 4){
            str = "You lose the game!\n";
        }
        else{
            str = to_string(a) + "A" + to_string(b) + "B\n";
        }
    }
    return str;
}
   
// Driver function
int main(int argc, char* argv[])
{
    int socketfdTCP, connfdTCP, socketfdUDP, max_client = 20;
    struct sockaddr_in serverInfo, clientInfo;

    //UDP socket
    socketfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfdUDP == -1){
        cout<<"Oops! UDP socket creation failed!\n";
        exit(0);
    }

    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[1]));
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
    //cout<<"port:"<<serverInfo.sin_port<<endl;

    if(bind(socketfdUDP, (struct sockaddr*)& serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! UDP socket bind failed!\n";
        exit(0);
    }
    cout<<"UDP server is running\n";

    //TCP socket
    socketfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfdTCP == -1){
        cout<<"Oops! TCP socket creation failed!\n";
        exit(0);
    }

    int optimal = 1;
    if(setsockopt(socketfdTCP, SOL_SOCKET, SO_REUSEADDR, &optimal, sizeof(int)) == -1){
        cout<<"Oops! TCP socket setup failed!\n";
    }

    //TCP bind 
    if(bind(socketfdTCP, (struct sockaddr*)& serverInfo, sizeof(serverInfo)) == -1){
        cout<<"Oops! socket bind failed!\n";
        exit(0);
    } 

    if(listen(socketfdTCP, max_client) == -1){
        cout<<"Oops! socket listen failed!\n";
        exit(0);
    }
    cout<<"TCP server is running\n";


    fd_set rfds;
    vector<int> clientSocketfd(max_client, 0);
    bzero(&clientInfo, sizeof(clientInfo));
    int len =sizeof(clientInfo);
    vector<USERS> userData;

    while(1){
        FD_ZERO(&rfds);
        //add the TCP & UDP master socket into rfds to monitor
        FD_SET(socketfdTCP, &rfds);
        FD_SET(socketfdUDP, &rfds);
        int maxfd = max(socketfdTCP, socketfdUDP);
        
        //add child to be monitor
        for(int i=0;i<max_client;i++){
            if(clientSocketfd[i] > 0){
                FD_SET(clientSocketfd[i], &rfds);
                if(maxfd < clientSocketfd[i]){
                    maxfd = clientSocketfd[i];
                }
            }
        }

        //monitoring
        int n =select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if(n == -1){
            cout<<"Oops! select failed!\n";
            exit(0);
        }

        //UDP
        if(FD_ISSET(socketfdUDP, &rfds)){
            char sendBuff[1024] = {0}, recvBuff[1024] = {0};
            recvfrom(socketfdUDP, recvBuff, sizeof(recvBuff), 0, (struct sockaddr*)& clientInfo, (socklen_t*)& len);
            cout<<"UDP get msg: "<<recvBuff<<endl;
            vector<string> v = split(recvBuff);
            if(v[0]=="register"){
                if(v.size() != 4){
                    strcpy(sendBuff, "Usage: register <username> <email> <password>\n");
                }
                else{
                    bool flag = false;
                    for(int i=0;i<userData.size();i++){
                        if(userData[i].name == v[1]){
                            strcpy(sendBuff, "Username is already used.\n");
                            flag =true;
                            break;
                        }
                        if(userData[i].email == v[2]){
                            strcpy(sendBuff, "Email is already used.\n");
                            flag = true;
                            break;
                        }
                    }
                    if(!flag){
                        strcpy(sendBuff, "Register successfully.\n");
                        USERS user;
                        user.name = v[1];
                        user.email = v[2];
                        user.password = v[3];
                        userData.push_back(user);
                    }
                }
                sendto(socketfdUDP, sendBuff, sizeof(sendBuff), 0, (struct sockaddr*)& clientInfo, len);
            }
            else if(v[0] == "game-rule"){
                strcpy(sendBuff, "1. Each question is a 4-digit secret number.\n2. After each guess, you will get a hint with the following information:\n2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n3. 5 chances for each question.\n");
                sendto(socketfdUDP, sendBuff, sizeof(sendBuff), 0, (struct sockaddr*)& clientInfo, len);
            }
            else{
                cout<<"Oops! somthing error in UDP!\n";
                exit(0);
            }
        }

        //TCP
        if(FD_ISSET(socketfdTCP, &rfds)){
            int serverlen = sizeof(serverInfo);
            int newSocket = accept(socketfdTCP, (struct sockaddr*)& serverInfo, (socklen_t*) & serverlen);
            if(newSocket == -1){
                cout<<"Oops! socket accept failed!\n";
                exit(0);
            }
            cout<<"New connection.\n";
            //add new socket into client
            for(int i=0;i<clientSocketfd.size();i++){
                if(clientSocketfd[i] == 0){
                    clientSocketfd[i] = newSocket;
                    break;
                }
            }
        }

        //check client
        for(int i=0;i<clientSocketfd.size();i++){
            char recvBuff[1024] = {0}, sendBuff[1024] = {0};
            if(FD_ISSET(clientSocketfd[i], &rfds)){
                if(recv(clientSocketfd[i], recvBuff, sizeof(recvBuff), 0) == 0){//exit
                    /*for(int j=0;j<userData.size();j++){
                        if(userData[j].pos == i){
                            userData[j].pos = -1;
                        }
                    }
                    close(clientSocketfd[i]);
                    clientSocketfd[i] = 0;*/
                }
                else{
                    cout<<"TCP get msg: "<<recvBuff<<endl;
                    vector<string> v = split(recvBuff);
                    if(v[0] == "login"){
                        if(v.size() != 3){
                            strcpy(sendBuff, "Usage: login <username> <password>\n");
                        }
                        else{
                            bool flag =false;
                            for(int j=0;j<userData.size();j++){
                                if(v[1] == userData[j].name){
                                    if(v[2] == userData[j].password){
                                        if(userData[j].pos != -1){
                                            strcpy(sendBuff, "Please logout first.\n");
                                        }
                                        else{
                                            string str = "Welcome, " + v[1] + ".\n";
                                            strcpy(sendBuff, str.c_str());
                                            userData[j].pos = i;
                                        }
                                    }
                                    else{
                                        strcpy(sendBuff, "Password not correct.\n");
                                    }
                                    flag = true;
                                    break;
                                }
                            }
                            if(!flag){
                                strcpy(sendBuff, "Username not found.\n");
                            }
                        }
                    }
                    else if(v[0] == "logout"){
                        bool flag =false;
                        for(int j=0;j< userData.size();j++){
                            if(userData[j].pos == i){
                                string str = "Bye, " + userData[j].name +".\n";
                                strcpy(sendBuff, str.c_str());
                                userData[j].pos = -1;
                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            strcpy(sendBuff, "Please login first.\n");
                        }
                    }
                    else if(v[0] == "exit"){
                        for(int j=0;j<userData.size();j++){
                            if(userData[j].pos == i){
                                userData[j].pos = -1;
                            }
                        }
                        close(clientSocketfd[i]);
                        clientSocketfd[i] = 0;
                        strcpy(sendBuff, " ");
                    }
                    else if(v[0] == "start-game"){
                        bool flag =false;
                        for(int j=0;j< userData.size();j++){
                            if(userData[j].pos == i){
                                if(v.size() == 1){
                                    srand(time(NULL));
                                    
                                    for(int i=0;i<4;i++){
                                        int x = rand() % (9 - 1 + 1) + 0;
                                        userData[j].ans += '0' + x;
                                    }
                                    strcpy(sendBuff, "Please typing a 4-digit number:\n");
                                }
                                else if(v.size() ==2){
                                    if(v[1].size() != 4){
                                        strcpy(sendBuff, "Usage: start-game <4-digit number>\n");
                                    }
                                    else{
                                        for(int k=0;k<4;k++){
                                            if(!isdigit(v[1][k])){
                                                strcpy(sendBuff, "Usage: start-game <4-digit number>\n");
                                                break;
                                            }
                                            if(k == 3){
                                                strcpy(sendBuff, "Please typing a 4-digit number:\n");
                                                userData[j].ans = v[1];
                                            }
                                        }
                                    }
                                }
                                else{
                                    strcpy(sendBuff, "Usage: start-game <4-digit number>\n");
                                }
                                flag = true;
                                break;
                            }
                        }
                        if(!flag){
                            strcpy(sendBuff, "Please login first.\n");
                        }
                    }
                    else{
                        for(int j=0;j<userData.size();j++){
                            if(i == userData[j].pos){
                                if(v[0].size() != 4){
                                    strcpy(sendBuff, "Your guess should be a 4-digit number.\n");
                                }
                                else{
                                    for(int k=0;k<4;k++){
                                        if(!isdigit(v[0][k])){
                                            strcpy(sendBuff, "Your guess should be a 4-digit number.\n");
                                            break;
                                        }
                                        if(k == 3){
                                            string str;
                                            if(userData[j].time < 5){
                                                str = game1A2B(userData[j].ans, v[0], userData[j].time);
                                                strcpy(sendBuff, str.c_str());
                                                userData[j].time++;
                                            }
                                            if(str[4] == 'l' || str[4] == 'g'){//lose or win
                                                userData[j].time = 0;
                                                userData[j].ans= "";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                    }

                    send(clientSocketfd[i], sendBuff, sizeof(sendBuff), 0);
                }
            }
        }

    }




    close(socketfdTCP);
    close(socketfdUDP);

    return 0;
}