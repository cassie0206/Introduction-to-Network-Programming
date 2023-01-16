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
#include <arpa/inet.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string.h>
#include <map>
#include <set>
#include <fstream>

using namespace std;

int curNum = 0;
int num;

typedef struct USERS{
    string name;
    string email;
    string password;
    int pos = -1;//keep login/logout
    string ans;//1A2B
    int time = 0;//for 1A2B timer
    int roomID = -1, code = -1;
    bool isManager = false;
    map<int, int> inviterRoom; // <id, code>
};

typedef struct ROOMS{
    int id = -1, code = -1;
    string manager;
    bool isOpen = true;
    vector<string> peer; // all client in the game room (include manager)
    string ans;
    int round=0;
    int turn = 0;
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
    string str = "";

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
        str = "Bingo!!!";
    }
    else{
        str = to_string(a) + "A" + to_string(b) + "B";
    }

    return str;
}

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
    //serverInfo.sin_port = htons(atoi(argv[1]));
    serverInfo.sin_port = htons(8888);
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);

    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    if(gethostname(hostbuffer, sizeof(hostbuffer)) == -1){
        cout<<"Oops! get host name failed!\n";
        exit(0);
    }

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    if(host_entry == NULL){
        cout<<"Oops! get host by name failed!\n";
        exit(0);
    }

    // To convert an Internet network address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    if(IPbuffer == NULL){
        cout<<"Oops! get ip address failed!\n";
        exit(0);
    }

    printf("Host IP: %s\n", IPbuffer);
    string ip = string(IPbuffer);
    num = ip[5] - '0';

    ofstream ofs;
    string file = "../efs/output" + to_string(num) + ".txt";
    ofs.open(file);
    if (!ofs.is_open()) {
        cout << "Failed to open file.\n";
        exit(0); // EXIT_FAILURE
    }
    ofs << "Server" << num << ": " << curNum << "\n";
    ofs.close();

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
    map<string, USERS> userData; // <name, user>
    map<int, ROOMS> roomData; // <roomID, room>

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
            int buffSize;
            n = recvfrom(socketfdUDP, recvBuff, sizeof(recvBuff), 0, (struct sockaddr*)& clientInfo, (socklen_t*)& len);
            recvBuff[n] = '\0';
            cout<<"UDP get msg: "<<recvBuff<<endl;
            vector<string> v = split(recvBuff);
            if(v[0]=="register"){
                if(v.size() != 4){
                    string str = "Usage: register <username> <email> <password>\n";
                    buffSize = str.size();
                    strcpy(sendBuff, "Usage: register <username> <email> <password>\n");
                }
                else{
                    bool flag = false;
                    for(auto it=userData.begin();it!=userData.end();it++){
                        if(it->second.name == v[1] || it->second.email == v[2]){
                            string str = "Username or Email is already used\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                            flag =true;
                            break;
                        }
                    }
                    if(!flag){
                        string str = "Register Successfully\n";
                        buffSize = str.size();
                        strcpy(sendBuff, str.c_str());
                        USERS user;
                        user.name = v[1];
                        user.email = v[2];
                        user.password = v[3];
                        userData.insert(pair<string, USERS>(v[1], user));
                    }
                }
            }
            else if(v[0] == "game-rule"){
                strcpy(sendBuff, "1. Each question is a 4-digit secret number.\n2. After each guess, you will get a hint with the following information:\n2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n3. 5 chances for each question.\n");
            }
            else if(v[0] == "list" && v[1] == "rooms"){
                string str = "List Game Rooms\n";
                int i = 1;
                if(roomData.empty()){
                    str += "No Rooms\n";
                }
                for(auto it = roomData.begin(); it != roomData.end(); it++){
                    str += to_string(i++);
                    if(it->second.code == -1){
                        str += ". (Public) Game Room ";
                    }
                    else{
                        str += ". (Private) Game Room ";
                    }
                    str += to_string(it->second.id);
                    if(it->second.isOpen){
                        str += " is open for players\n";
                    }
                    else{
                        str += " has started playing\n";
                    }                   
                }    
                buffSize = str.size();
                strcpy(sendBuff, str.c_str());
            }
            else if(v[0] =="list" && v[1] == "users"){
                string str = "List Users\n";
                int i = 1;
                if(userData.empty()){
                    str += "No Users\n";
                }
                for(auto it = userData.begin(); it != userData.end(); it++){
                    str += to_string(i++);
                    str += (". " + it->second.name + "<" + it->second.email + "> ");
                    if(it->second.pos == -1){
                        str += "Offline\n";
                    }
                    else{
                        str += "Online\n";
                    }
                }
                buffSize = str.size();
                strcpy(sendBuff, str.c_str());
            }
            else{
                cout<<"Oops! somthing error in UDP!\n";
                exit(0);
            }

            sendto(socketfdUDP, sendBuff, buffSize, 0, (struct sockaddr*)& clientInfo, len);
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
            int buffSize;
            bool sended = false;
            if(FD_ISSET(clientSocketfd[i], &rfds)){
                if(recv(clientSocketfd[i], recvBuff, sizeof(recvBuff), 0) == 0){// exit
                    //getpeername(clientSocketfd[i] , (struct sockaddr*)&serverInfo , (socklen_t*)& serverlen);  
                    //printf("user%d %s:%d disconnected\n" , userData[i], inet_ntoa(serverInfo.sin_addr) , ntohs(serverInfo.sin_port));  
                    for(auto it=userData.begin();it!=userData.end();it++){
                        if(it->second.pos == i){
                            if(it->second.roomID != -1 && it->second.isManager == true){
                                roomData.erase(roomData.find(it->second.roomID));
                                it->second.isManager = false;
                            }
                            if(it->second.pos != -1){
                                curNum--;
                                ofstream ofs;
                                string file = "../efs/output" + to_string(num) + ".txt";
                                ofs.open(file);
                                if (!ofs.is_open()) {
                                    cout << "Failed to open file.\n";
                                    exit(0); // EXIT_FAILURE
                                }
                                ofs << "Server" << num << ": " << curNum << "\n";
                                ofs.close();
                            }

                            it->second.pos = -1;
                            it->second.roomID = -1;
                            it->second.code = -1;
                            it->second.time = 0;
                        }
                    }

                    close(clientSocketfd[i]);
                    clientSocketfd[i] = 0;
                    strcpy(sendBuff, " ");
                }
                else{
                    cout<<"TCP get msg: "<<recvBuff<<endl;
                    vector<string> v = split(recvBuff);
                    if(v[0] == "login"){
                        if(v.size() != 3){
                            string str = "Usage: login <username> <password>\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                        else{
                            bool flag =false;
                            for(auto it=userData.begin();it!=userData.end();it++){
                                if(v[1] == it->second.name){
                                    if(it->second.pos != -1 && it->second.pos != i){
                                        string str = "Someone already logged in as " + v[1] + "\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                        flag = true;
                                        break;
                                    }
                                    v[2].erase(v[2].end() - 1);
                                    if(it->second.password != v[2]){
                                        string str = "Wrong password\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else{
                                        bool login=false;
                                        for(auto iter=userData.begin();iter!=userData.end();iter++){
                                            if(iter->second.pos == i){
                                                string str = "You already logged in as " + iter->second.name + "\n";
                                                buffSize = str.size();
                                                strcpy(sendBuff, str.c_str());
                                                login=true;
                                                break;
                                            }
                                        }
                                        if(!login){
                                            string str = "Welcome, " + v[1] + "\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            it->second.pos = i;
                                            curNum++;
                                            ofstream ofs;
                                            string file = "../efs/output" + to_string(num) + ".txt";
                                            ofs.open(file);
                                            if (!ofs.is_open()) {
                                                cout << "Failed to open file.\n";
                                                exit(0); // EXIT_FAILURE
                                            }
                                            ofs << "Server" << num << ": " << curNum << "\n";
                                            ofs.close();
                                        }
                                    }
                                
                                    flag = true;
                                    break;
                                }
                            }
                            if(!flag){
                                string str = "Username does not exist\n";
                                buffSize = str.size();
                                strcpy(sendBuff, str.c_str());
                            }
                        }
                    }
                    else if(v[0] == "logout\n"){
                        bool flag =false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){
                                if(it->second.roomID != -1){
                                    string str = "You are already in game room " + to_string(it->second.roomID) +", please leave game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    string str = "Goodbye, " + it->second.name +"\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                    it->second.pos = -1;
                                    curNum--;
                                    ofstream ofs;
                                    string file = "../efs/output" + to_string(num) + ".txt";
                                    ofs.open(file);
                                    if (!ofs.is_open()) {
                                        cout << "Failed to open file.\n";
                                        exit(0); // EXIT_FAILURE
                                    }
                                    ofs << "Server" << num << ": " << curNum << "\n";
                                    ofs.close();
                                }
                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "status\n"){
                        string str;
                        for(int i=1;i<4;i++){
                            string filename = "../efs/output" + to_string(i) + ".txt";
                            ifstream f;
                            string line;
                            
                            f.open(filename);
                            getline(f, line);
                            str += line + "\n";
                            f.close();
                        }
                        cout<<"status: "<<str<<endl;
                        buffSize = str.size();
                        strcpy(sendBuff, str.c_str());
                    }
                    else if(v[0] == "create"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID != -1){// in game room
                                    cout<<"already in\n";
                                    string str = "You are already in game room " + to_string(it->second.roomID) +", please leave game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    auto iter = roomData.find(stoi(v[3]));
                                    if(iter != roomData.end()){
                                        string str = "Game room ID is used, choose another one\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else{
                                        if(v[1] == "public"){
                                            string str = "You create public game room " + v[3];
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            it->second.roomID = stoi(v[3]);
                                            it->second.isManager = true;
                                            ROOMS room;
                                            room.id = stoi(v[3]);
                                            room.manager = it->second.name;
                                            room.peer.push_back(it->second.name);
                                            roomData.insert(pair<int, ROOMS>(stoi(v[3]), room));
                                        }
                                        else if(v[1] == "private"){
                                            string str = "You create private game room " + v[3] +"\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            it->second.isManager = true;
                                            it->second.roomID = stoi(v[3]);
                                            it->second.code = stoi(v[4]);
                                            ROOMS room;
                                            room.id = stoi(v[3]);
                                            room.code = stoi(v[4]);
                                            room.manager = it->second.name;
                                            room.peer.push_back(it->second.name);
                                            roomData.insert(pair<int, ROOMS>(stoi(v[3]), room));
                                        }
                                    }
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "join"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                auto iter = roomData.find(stoi(v[2]));
                                v[2].erase(v[2].end() - 1);
                                if(iter != roomData.end()){
                                    if(iter->second.id == it->second.roomID){
                                        string str = "You are already in game room " + v[2] + ", please leave game room\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else if(iter->second.code != -1){
                                        string str = "Game room is private, please join game by invitation code\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else if(!iter->second.isOpen){
                                        string str = "Game has started, you can't join now\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else{
                                        string str = "You join game room " + v[2] + "\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                        send(clientSocketfd[i], sendBuff, buffSize, 0);
                                        sended = true;

                                        for(int j=0;j<iter->second.peer.size();j++){
                                            if(iter->second.peer[j] == it->second.name){
                                                continue;
                                            }
                                            char peerBuff[1024] = {};
                                            string str = "Welcome, " + it->second.name + " to game!\n";
                                            int peerBuffSize = str.size();
                                            strcpy(peerBuff, str.c_str());
                                            send(clientSocketfd[userData[iter->second.peer[j]].pos], peerBuff, peerBuffSize, 0);
                                        }

                                        it->second.roomID = stoi(v[2]);
                                        iter->second.peer.push_back(it->second.name);
                                    }
                                    
                                }
                                else{
                                    string str = "Game room " + v[2] +" is not exist\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "invite"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID == -1){
                                    string str = "You did not join any game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    if(it->second.isManager == false || it->second.code == -1){
                                        string str = "You are not private game room manager\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                    else{
                                        v[1].erase(v[1].end() - 1);
                                        for(auto iter=userData.begin();iter!=userData.end();iter++){
                                            if(iter->second.email == v[1]){
                                                if(iter->second.pos == -1){
                                                    cout<<"sfwsef\n";
                                                    string str = "Invitee not logged in\n";
                                                    buffSize = str.size();
                                                    strcpy(sendBuff, str.c_str());
                                                }
                                                else{
                                                    string str = "You send invitation to " + iter->second.name + "<" +iter->second.email + ">\n";
                                                    buffSize = str.size();
                                                    strcpy(sendBuff, str.c_str());
                                                    send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                    sended = true;

                                                    char inviteeBuff[1024] = {0};
                                                    str = "You receive invitation from " + it->second.name + "<" + it->second.email + ">\n";
                                                    int inviteeBuffSize = str.size();
                                                    strcpy(inviteeBuff, str.c_str());
                                                    send(clientSocketfd[iter->second.pos], inviteeBuff, inviteeBuffSize, 0);
                                                    iter->second.inviterRoom.insert(pair<int, int>(it->second.roomID, it->second.code));
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "list" && v[1] == "invitations\n"){
                        string str = "List invitations\n";
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.inviterRoom.size() == 0){
                                    str += "No Invitations\n";
                                    break;
                                }
                                int j = 1;
                                for(auto iter=it->second.inviterRoom.begin();iter!=it->second.inviterRoom.end();iter++){  
                                    auto itInviter = roomData.find(iter->first);
                                    if(itInviter != roomData.end()){
                                        str += to_string(j++) + ". " + roomData[iter->first].manager + "<" + userData[roomData[iter->first].manager].email + ">" + " invite you to join game room " + to_string(iter->first) + ", invitation code is " + to_string(iter->second) + "\n";
                                    }
                                }
                                break;
                            }
                        }
                        buffSize = str.size();
                        strcpy(sendBuff, str.c_str());
                    }
                    else if(v[0] == "accept"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID != -1){
                                    string str = "You are already in game room " + to_string(it->second.roomID) + ", please leave game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    bool flag1 = false;
                                    for(auto iter=userData.begin();iter!=userData.end();iter++){
                                        if(iter->second.email == v[1] && iter->second.code != stoi(v[2])){
                                            string str = "Your invitation code is incorrect\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            flag1 = true;
                                            break;
                                        }
                                    }
                                    if(!flag1){
                                        bool flag2=false;
                                        for(auto iter=it->second.inviterRoom.begin();iter!=it->second.inviterRoom.end();iter++){
                                        if(iter->second == stoi(v[2])){// find the invitation
                                            auto itAccept = roomData.find(iter->first);
                                            if(itAccept != roomData.end()){
                                                if(itAccept->second.isOpen){
                                                    string str = "You join game room " + to_string(iter->first) + "\n";
                                                    buffSize = str.size();
                                                    strcpy(sendBuff, str.c_str());
                                                    send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                    sended=true;

                                                    for(int j=0;j<roomData[iter->first].peer.size();j++){
                                                        if(roomData[iter->first].peer[j] == it->second.name){
                                                            continue;
                                                        }
                                                        char peerBuff[1024] = {0};
                                                        string str = "Welcome, " + it->second.name + " to game!\n";
                                                        int peerBuffSize = str.size();
                                                        strcpy(peerBuff, str.c_str());
                                                        send(clientSocketfd[userData[roomData[iter->first].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                    }

                                                    roomData[iter->first].peer.push_back(it->second.name);
                                                    it->second.code = stoi(v[2]);
                                                    it->second.roomID = iter->first;
                                                }
                                                else{
                                                    string str = "Game has started, you can't join now\n";
                                                    buffSize = str.size();
                                                    strcpy(sendBuff, str.c_str());
                                                }
                                            }
                                            else{// invitee is invited, but inviter leave the game room
                                                string str = "Invitation not exist\n";
                                                buffSize = str.size();
                                                strcpy(sendBuff, str.c_str());
                                            }
                                            flag2=true;
                                            break;
                                        }
                                        }
                                        if(!flag2){//invitee is not invited
                                            string str = "Invitation not exist\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                        }
                                    }
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "leave"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID == -1){
                                    string str = "You did not join any game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    if(it->second.isManager){
                                        string str = "You leave game room " + to_string(it->second.roomID) + "\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                        send(clientSocketfd[i], sendBuff, buffSize, 0);
                                        sended = true;

                                        for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                            if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                continue;
                                            }
                                            char peerBuff[1024] = {0};
                                            string str = "Game room manager leave game room " + to_string(it->second.roomID) + ", you are forced to leave too\n";
                                            int peerBuffSize = str.size();
                                            strcpy(peerBuff, str.c_str());
                                            send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                            userData[roomData[it->second.roomID].peer[j]].roomID = -1;
                                            userData[roomData[it->second.roomID].peer[j]].code = -1;
                                            userData[roomData[it->second.roomID].peer[j]].time = 0;
                                            userData[roomData[it->second.roomID].peer[j]].ans = "";
                                        }

                                        roomData.erase(it->second.roomID);
                                        it->second.roomID = -1;
                                        it->second.code = -1;
                                        it->second.isManager = false;
                                        it->second.time = 0;
                                        it->second.ans = "";
                                    }
                                    else{
                                        if(roomData[it->second.roomID].isOpen){
                                            string str = "You leave game room " + to_string(it->second.roomID) + "\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            send(clientSocketfd[i], sendBuff, buffSize, 0);
                                            sended = true;

                                            for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                    continue;
                                                }
                                                char peerBuff[1024] = {0};
                                                string str = it->second.name + " leave game room " + to_string(it->second.roomID) + "\n";
                                                int peerBuffSize = str.size();
                                                strcpy(peerBuff, str.c_str());
                                                send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                            }

                                            auto iter = find(roomData[it->second.roomID].peer.begin(), roomData[it->second.roomID].peer.end(), it->second.name);
                                            roomData[it->second.roomID].peer.erase(iter);
                                            it->second.roomID = -1;
                                            it->second.code = -1;
                                            it->second.time = 0;
                                            it->second.ans = "";
                                        }
                                        else{//started
                                            string str = "You leave game room " + to_string(it->second.roomID) + ", game ends\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                            send(clientSocketfd[i], sendBuff, buffSize, 0);
                                            sended = true;

                                            for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                    continue;
                                                }
                                                char peerBuff[1024] = {0};
                                                string str = it->second.name + " leave game room " + to_string(it->second.roomID) + ", game ends\n";
                                                int peerBuffSize = str.size();
                                                strcpy(peerBuff, str.c_str());
                                                send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                            }

                                            auto iter = find(roomData[it->second.roomID].peer.begin(), roomData[it->second.roomID].peer.end(), it->second.name);
                                            roomData[it->second.roomID].peer.erase(iter);
                                            roomData[it->second.roomID].isOpen = true;
                                            roomData[it->second.roomID].ans = "";
                                            roomData[it->second.roomID].round = 0;
                                            roomData[it->second.roomID].turn = 0;
                                            it->second.roomID = -1;
                                            it->second.code = -1;
                                            it->second.time = 0;
                                            it->second.ans = "";
                                        }
                                    }
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "start"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID == -1){
                                    string str = "You did not join any game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    if(it->second.isManager){
                                        if(roomData[it->second.roomID].isOpen){
                                            if(v.size() == 4){
                                                v[3].erase(v[3].end() - 1);
                                                if(v[3].size() != 4){
                                                    string str = "Please enter 4 digit number with leading zero\n";
                                                    buffSize = str.size();
                                                    strcpy(sendBuff, str.c_str());
                                                }
                                                else{
                                                    for(int k=0;k<4;k++){
                                                        if(!isdigit(v[3][k])){
                                                            strcpy(sendBuff, "Please enter 4 digit number with leading zero\n");
                                                            break;
                                                        }
                                                        if(k == 3){
                                                            string str = "Game start! Current player is " + it->second.name + "\n";
                                                            buffSize = str.size();
                                                            strcpy(sendBuff, str.c_str());
                                                            send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                            sended = true;

                                                            for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                                if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                                    continue;
                                                                }
                                                                char peerBuff[1024] = {0};
                                                                string str = "Game start! Current player is " + it->second.name + "\n";
                                                                int peerBuffSize = str.size();
                                                                strcpy(peerBuff, str.c_str());
                                                                send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                                userData[roomData[it->second.roomID].peer[j]].time = 0;
                                                            }

                                                            roomData[it->second.roomID].ans = v[3];
                                                            roomData[it->second.roomID].turn = 0;
                                                            roomData[it->second.roomID].round = stoi(v[2]);
                                                            roomData[it->second.roomID].isOpen = false;
                                                        }
                                                    }
                                                }
                                            }
                                            else{ // random
                                                srand(time(NULL));
                                    
                                                for(int i=0;i<4;i++){
                                                    int x = rand() % (9 - 1 + 1) + 0;
                                                    roomData[it->second.roomID].ans += '0' + x;
                                                }
                                                string str = "Game start! Current player is " + it->second.name + "\n";
                                                buffSize = str.size();
                                                strcpy(sendBuff, str.c_str());
                                                send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                sended = true;

                                                for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                    if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                        continue;
                                                    }
                                                    char peerBuff[1024] = {0};
                                                    string str = "Game start! Current player is " + it->second.name + "\n";
                                                    int peerBuffSize = str.size();
                                                    strcpy(peerBuff, str.c_str());
                                                    send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                    userData[roomData[it->second.roomID].peer[j]].time = 0;
                                                }
                                                roomData[it->second.roomID].turn = 0;
                                                roomData[it->second.roomID].round = stoi(v[2]);
                                                roomData[it->second.roomID].isOpen = false;
                                            }
                                        }
                                        else{
                                            string str = "Game has started, you can't start again\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                        }
                                    }
                                    else{
                                        string str = "You are not game room manager, you can't start game\n";
                                        buffSize = str.size();
                                        strcpy(sendBuff, str.c_str());
                                    }
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    else if(v[0] == "guess"){
                        bool flag = false;
                        for(auto it=userData.begin();it!=userData.end();it++){
                            if(it->second.pos == i){ //login
                                if(it->second.roomID == -1){
                                    string str = "You did not join any game room\n";
                                    buffSize = str.size();
                                    strcpy(sendBuff, str.c_str());
                                }
                                else{
                                    if(roomData[it->second.roomID].isOpen){
                                        if(it->second.isManager){
                                            string str = "You are game room manager, please start game first\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                        }
                                        else{
                                            string str = "Game has not started yet\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                        }
                                    }
                                    else{
                                        if(roomData[it->second.roomID].peer[roomData[it->second.roomID].turn] != it->second.name){
                                            string str = "Please wait..., current player is " + roomData[it->second.roomID].peer[roomData[it->second.roomID].turn] + "\n";
                                            buffSize = str.size();
                                            strcpy(sendBuff, str.c_str());
                                        }
                                        else{
                                            v[1].erase(v[1].end() - 1);
                                            if(v[1].size() != 4){
                                                string str = "Please enter 4 digit number with leading zero\n";
                                                buffSize = str.size();
                                                strcpy(sendBuff, str.c_str());
                                            }
                                            else{
                                                for(int k=0;k<4;k++){
                                                    if(!isdigit(v[1][k])){
                                                        string str = "Please enter 4 digit number with leading zero\n";
                                                        buffSize = str.size();
                                                        strcpy(sendBuff, str.c_str());
                                                        break;
                                                    }
                                                    if(k == 3){
                                                        string str;
                                                        if(it->second.time < roomData[it->second.roomID].round){
                                                            string guess = game1A2B(roomData[it->second.roomID].ans, v[1], it->second.time);
                                                            if(guess == "Bingo!!!"){
                                                                string str = it->second.name + " guess '" + v[1] + "' and got Bingo!!! " + it->second.name + " wins the game, game ends\n";
                                                                buffSize = str.size();
                                                                strcpy(sendBuff, str.c_str());
                                                                send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                                sended = true;

                                                                for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                                    if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                                        continue;
                                                                    }
                                                                    char peerBuff[1024] = {0};
                                                                    int peerBuffSize = str.size();
                                                                    strcpy(peerBuff, str.c_str());
                                                                    send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                                    userData[roomData[it->second.roomID].peer[j]].time = 0;
                                                                }

                                                                it->second.time = 0;
                                                                roomData[it->second.roomID].ans = "";
                                                                roomData[it->second.roomID].turn = 0;
                                                                roomData[it->second.roomID].isOpen = true;
                                                            }
                                                            else{
                                                                if(it->second.time == roomData[it->second.roomID].round - 1 && roomData[it->second.roomID].turn == roomData[it->second.roomID].peer.size() - 1){
                                                                    string str = it->second.name + " guess '" + v[1] + "' and got '" + guess + "'\nGame ends, no one wins\n";
                                                                    buffSize = str.size();
                                                                    strcpy(sendBuff, str.c_str());
                                                                    send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                                    sended = true;
                                                                    
                                                                    for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                                        if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                                            continue;
                                                                        }
                                                                        char peerBuff[1024] = {0};
                                                                        int peerBuffSize = str.size();
                                                                        strcpy(peerBuff, str.c_str());
                                                                        send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                                        userData[roomData[it->second.roomID].peer[j]].time = 0;
                                                                    }

                                                                    it->second.time = 0;
                                                                    roomData[it->second.roomID].ans = "";
                                                                    roomData[it->second.roomID].turn = 0;
                                                                    roomData[it->second.roomID].isOpen = true;
                                                                }
                                                                else{
                                                                    string str = it->second.name + " guess '" + v[1] + "' and got '" + guess + "'\n";
                                                                    buffSize = str.size();
                                                                    strcpy(sendBuff, str.c_str());
                                                                    send(clientSocketfd[i], sendBuff, buffSize, 0);
                                                                    sended = true;

                                                                    for(int j=0;j<roomData[it->second.roomID].peer.size();j++){
                                                                        if(roomData[it->second.roomID].peer[j] == it->second.name){
                                                                            continue;
                                                                        }
                                                                        char peerBuff[1024] = {0};
                                                                        int peerBuffSize = str.size();
                                                                        strcpy(peerBuff, str.c_str());
                                                                        send(clientSocketfd[userData[roomData[it->second.roomID].peer[j]].pos], peerBuff, peerBuffSize, 0);
                                                                    }

                                                                    it->second.time++;
                                                                    roomData[it->second.roomID].turn++;
                                                                    roomData[it->second.roomID].turn %= roomData[it->second.roomID].peer.size();
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                flag =true;
                                break;
                            }
                        }
                        if(!flag){
                            string str = "You are not logged in\n";
                            buffSize = str.size();
                            strcpy(sendBuff, str.c_str());
                        }
                    }
                    if(!sended){
                        send(clientSocketfd[i], sendBuff, buffSize, 0);
                    }
                }
            }
        }

    }


    close(socketfdTCP);
    close(socketfdUDP);

    return 0;
}