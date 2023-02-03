#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <random>
#include <queue>
#include <chrono>
#include <thread>

using namespace std;
using std::this_thread::sleep_for;

#define PORT1	 11005
#define PORT2	 11006
#define lineLength 2000
#define PACKET_BUFF_SIZE 5
#define MIN 11
#define MAX 20



int sockSD1, sockSD2;
char buffer[lineLength], recA[lineLength], recB[lineLength];
struct sockaddr_in serverAddr1, clientAddr1;
struct sockaddr_in serverAddr2, clientAddr2;
int sent = 0;

int dropProbability = 10;

std::uniform_int_distribution<> distrib(1, 10);

std::random_device rnd;
std::mt19937 gen(rnd());
pthread_t tid[3];
pthread_mutex_t lock;

struct Packet {
    Packet(bool is_ack_, int SN_, bool src_, bool dest_, string body_):
            is_ack(is_ack_), SN(SN_), src(src_), dest(dest_), body(body_) {};

    bool is_ack;
    int SN;
    bool src;
    bool dest;
    string body;
};
queue<Packet> packetBuffer;




void* receiveFromA(void* args);

void* receiveFromB(void* args);

void updateDropProbability();

vector<int> FrameContent(char buffer[]);

void* sendPackets(void* args);

void addPacket(string mssg1, string mssg2, string mssg3, int sender, char recBuffer[]);

string convertToString(char buff[], int size);



int main() {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    std::cout << "Router is Running..." << endl;


    if((sockSD1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if((sockSD2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr1, 0, sizeof(serverAddr1));
    memset(&clientAddr1, 0, sizeof(clientAddr1));

    memset(&serverAddr2, 0, sizeof(serverAddr2));
    memset(&clientAddr2, 0, sizeof(clientAddr2));

    serverAddr1.sin_family = AF_INET; // IPv4
    serverAddr1.sin_addr.s_addr = INADDR_ANY;
    serverAddr1.sin_port = htons(PORT1);

    serverAddr2.sin_family = AF_INET; // IPv4
    serverAddr2.sin_addr.s_addr = INADDR_ANY;
    serverAddr2.sin_port = htons(PORT2);

    if (bind(sockSD1, (const struct sockaddr *)&serverAddr1, sizeof(serverAddr1)) < 0)
    {
        perror("bind1 failed");
        exit(EXIT_FAILURE);
    }

    buffer[1500] = '\0';

    memset(&buffer, 0, sizeof(buffer));


    // making threads for sending and receiving
    pthread_create(&tid[0], nullptr, sendPackets, nullptr);
    pthread_create(&tid[1], nullptr, receiveFromA, nullptr);
    pthread_create(&tid[2], nullptr, receiveFromB, nullptr);

    // waiting for send tread to close
    pthread_join(tid[0], NULL);
    pthread_cancel(tid[1]);
    pthread_cancel(tid[2]);

    close(sockSD2);
    return 0;
}



void updateDropProbability() {
    dropProbability = packetBuffer.size() >= MAX ? 10 :
                      packetBuffer.size() - MIN > 5 ? (packetBuffer.size() - MIN) / 2 + 5 :
                      (packetBuffer.size() - MIN) / 2;
}

vector<int> FrameContent(char buffer[]) {
    try{
        string contS[3] = {""};
        int start = 0;
        for(int j = 0; j < 3; j++)
            for(int i = start; i < lineLength; i++) {
                if(buffer[i] == ' ') {
                    start = i+1;
                    break;
                }
                contS[j] += buffer[i];
            }

        return vector<int>{stoi(contS[0]), stoi(contS[1]), stoi(contS[2])};
    } catch(...) {
        abort();
    }

}

void* sendPackets(void* args) {
    while(true) {
        cerr << "size is " << packetBuffer.size() << endl;
        while(packetBuffer.empty());
        Packet p = packetBuffer.front();
        if(p.dest) {
            sendto(sockSD2, (char*)p.body.c_str(), lineLength,
                   MSG_CONFIRM, (const struct sockaddr *) &serverAddr2,
                   sizeof(clientAddr2));
        } else {
            sendto(sockSD1, (char*)p.body.c_str(), lineLength,
                   MSG_CONFIRM, (const struct sockaddr *) &clientAddr1,
                   sizeof(clientAddr1));
        }
        packetBuffer.pop();

        sent++;
        sleep_for(std::chrono::milliseconds(2));
    }
    return nullptr;
}

void addPacket(string mssg1, string mssg2, string mssg3, int sender, char recBuffer[]) {
    char* temp = new char[lineLength];
    memcpy(temp, recBuffer, lineLength - 1);
    updateDropProbability();
    vector<int> frameInfo = FrameContent(temp);
    int frameSeqNum = frameInfo[0];
    int destination = frameInfo[1];
    int size = frameInfo[2];

    int loss = distrib(gen);
    if(loss <= dropProbability) {
        std::cout << mssg3 << frameSeqNum << " from source A" << destination << endl;
        return;
    }

    if(packetBuffer.size() < PACKET_BUFF_SIZE) {
        if(!sender)
            std::cout << mssg1 << frameSeqNum << mssg2 << destination << "to queue..." << endl;
        else
            std::cout << mssg1 << frameSeqNum << " of source A" << destination << mssg2 << "to queue..." << endl;

        pthread_mutex_lock(&lock);
        packetBuffer.push(Packet(0, frameSeqNum, sender, !sender, convertToString(temp, size)));
        pthread_mutex_unlock(&lock);
    } else
        std::cout << mssg3 << frameSeqNum << " from source A" << destination << endl;
}

void* receiveFromA(void* args) {
    socklen_t len1 = sizeof(clientAddr1);
    while(true) {
        memset(&recA, 0, sizeof(recA));
        recvfrom(sockSD1, (char *)recA, lineLength,
                 MSG_WAITALL, ( struct sockaddr *) &clientAddr1,
                 (socklen_t*)&len1);

        addPacket("Adding packet ", " from A",
                  "dropping data packet ", 0, recA);
    }
    return nullptr;
}

void* receiveFromB(void* args) {
    socklen_t len2 = sizeof(clientAddr2);
    while(true) {
        memset(&recB, 0, sizeof(recB));
        recvfrom(sockSD2, recB, lineLength,
                 MSG_WAITALL, ( struct sockaddr *) &serverAddr2,
                 (socklen_t*)&len2);

        addPacket("Adding ack for packet ", " from B ",
                  "dropping ack packet for packet ", 1, recB);
    }
    return nullptr;
}

string convertToString(char buff[], int size) {
    string s = "";
    for(int i = 0; i < size; i++)
        s += buff[i];
    return s;
}