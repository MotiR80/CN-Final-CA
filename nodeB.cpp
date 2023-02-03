#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;

#define PORT	 11005
#define lineLength 1500
#define BUFF_SIZE 2000
#define NUM_OF_HOST 20

vector<int> lfr(NUM_OF_HOST + 1, 0);
string file_content;


void getFrameContent(char buffer[], int size);

vector<int> FrameContent(char buffer[]);


int main() {
    int sockfd;
    char buffer[BUFF_SIZE];
    struct sockaddr_in serverAddr, clientAddr;
    cout << "PC B is Running...\n";

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;
    buffer[2048] = '\0';
    len = sizeof(clientAddr);


    while(true) {
        memset(&buffer, '$', sizeof(buffer));

        recvfrom(sockfd, (char *)buffer, BUFF_SIZE,
                 MSG_WAITALL, ( struct sockaddr *) &clientAddr,
                 (socklen_t*)&len);

        vector<int> packetInfo = FrameContent(buffer);
        int frameSeqNum = packetInfo[0];
        int destination = packetInfo[1];
        int packetSize = packetInfo[2];


        if(frameSeqNum <= lfr[destination] + 1) {
            int n = to_string(frameSeqNum).length() + to_string(destination).length() + 2;
            n += to_string(n).length();
            char *ack =  (char*)((to_string(frameSeqNum) + " " + to_string(destination) + " " + to_string(n)).c_str());

            sendto(sockfd, ack, strlen(ack),
                   MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
                   len);
            if(frameSeqNum == lfr[destination] + 1) {
                lfr[destination]++;
                getFrameContent(buffer, packetSize);
                cout << "ack sent for packet " << frameSeqNum << endl;
            } else
                cout << "ack resent for packet " << frameSeqNum << endl;

        }
        cout << endl;
        ofstream MyFile("output.txt");
        MyFile << file_content;
        MyFile.close();
    }

}

vector<int> FrameContent(char buffer[]) {
    cout << "frame i got was" << buffer << endl;
    string contS[3] = {""};
    int start = 0;
    for(int j = 0; j < 3; j++)
        for(int i = start; i < BUFF_SIZE; i++) {
            if(buffer[i] == ' ') {
                start = i+1;
                break;
            }
            contS[j] += buffer[i];
        }
    return vector<int>{stoi(contS[0]), stoi(contS[1]), stoi(contS[2])};
}

void getFrameContent(char buffer[], int size) {
    int cnt = 0;
    for(int i = 0; i < size; i++){
        if (cnt < 3 || buffer[i] == ' '){
            if (buffer[i] == ' ')
                cnt++;
            continue;
        }
        file_content += buffer[i];
    }
    file_content += '\n';
}