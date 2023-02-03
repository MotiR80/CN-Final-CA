#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <thread>
#include <csignal>
#include <fstream>
#include <sys/time.h>
#include <algorithm>
#include <queue>
#include <chrono>
using namespace std::chrono;
using namespace std;

#define PORT     11005
#define lineLength 1500
#define BUFF_SIZE 2048
#define SWS 1
#define INTERVAL 150
#define INFINITE 1e9


pthread_t tid;

int currentSWS = SWS;
int ssthresh = INFINITE;
int maxSWS = -INFINITE;

int sockSD;
char buffer[BUFF_SIZE];
struct sockaddr_in servAddr;
vector<string> frames;
vector<int> unReceivedAcks;
bool lastAckReceived = false;
int nHost;
int nTimeout = 0;

int lar = 0, lfs = 0;

vector<int> FrameContent(char buffer[]);

string readFile(string fileName);

vector<string> makeFrame(int frameSize, string input);

void* recieveAck(void* args);

void timeoutHandle(int a);

int main() {

    nHost = 1;

    if ((sockSD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    int n;
    socklen_t len;

    string input = readFile("out-1MB.dt");
    frames = makeFrame(lineLength, input);



    auto start = high_resolution_clock::now();
    pthread_create(&tid, nullptr, recieveAck, nullptr);


    while(!lastAckReceived) {
        while(lfs >= frames.size())
            continue;
        if(lfs-lar < currentSWS) {
            string SNS = to_string(lfs+1);

            int n = to_string(nHost).length() + SNS.length() + frames[lfs].length() + 3;

            n += to_string(n).length();
            char char_array[n+1] = {0};
            strcpy(char_array, (SNS + " " + to_string(nHost) + " " + to_string(n) + " " + frames[lfs]).c_str());

            sendto(sockSD, char_array, sizeof(char_array),
                   MSG_CONFIRM, (const struct sockaddr *) &servAddr,
                   sizeof(servAddr));

            lfs++;
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    pthread_join(tid, nullptr);
    close(sockSD);
    return 0;
}

vector<int> FrameContent(char buffer[]) {
    string ContS[2] = {""};
    int start = 0;
    for(int j = 0; j < 2; j++)
        for(int i = start; i < lineLength; i++) {
            if(buffer[i] == ' ') {
                start = i+1;
                break;
            }
            ContS[j] += buffer[i];
        }

    return vector<int>{stoi(ContS[0]), stoi(ContS[1])};
}

string readFile(string fileName) {
    string line;
    string input = "";
    ifstream file(fileName);
    while(getline(file, line)) {
        input += line + "\n";
    }
    file.close();
    input.pop_back();
    return input;
}

void* recieveAck(void* args) {
    while(true) {
        struct itimerval it_val;
        it_val.it_value.tv_sec = INTERVAL/1000;
        it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;
        it_val.it_interval = it_val.it_value;
        memset(&buffer, 0, sizeof(buffer));
        signal(SIGALRM, timeoutHandle);
        setitimer(ITIMER_REAL, &it_val, NULL);
        recvfrom(sockSD, (char *)buffer, BUFF_SIZE + 1,
                 MSG_WAITALL, (struct sockaddr *) &servAddr,
                 (socklen_t*)&servAddr);

        vector<int> frameInfo = FrameContent(buffer);
        int SN = frameInfo[0];
        int dest = frameInfo[1];

        // Part 2
        if(dest != nHost)
            continue;

        cout << "ack received for packet " << atoi(buffer) << endl << endl;
        if(SN == frames.size() && lar+1 == frames.size()) {
            lastAckReceived = true;
            break;
        }
        if(lar+1 == SN) { //if ACK happens
            cout << "sliding the window to the right...\n";
            if(currentSWS < ssthresh){
                currentSWS *= 2;
                maxSWS = max(maxSWS, currentSWS);
            }
            else
                currentSWS++;
            nTimeout = 0;
            sort(unReceivedAcks.begin(), unReceivedAcks.end(), greater <>());
            lar++;
            for(int i = unReceivedAcks.size() - 1; i >= 0; i--)
                if(lar+1 == unReceivedAcks[i]) {
                    lar++;
                    unReceivedAcks.pop_back();
                }
        } else {
            unReceivedAcks.push_back(SN);
        }
    }
    return nullptr;
}

void timeoutHandle(int a) {
    cout << "Time Out Occured!\n\n";
    nTimeout++;
    if(nTimeout >= 1) {
        ssthresh = currentSWS / 2;
        currentSWS = 1;
    }
    unReceivedAcks.clear();
    lfs = lar;
}

vector<string> makeFrame(int frameSize, string input) {
    vector<string> frames;
    int i = 0;
    for(i = 0; i < input.size(); i += frameSize)
        frames.push_back(input.substr(i, frameSize - 1));

    if (i < input.size() - 1){
        i -= frameSize;
        frames.push_back(input.substr(i, input.size() - i - 1));
    }
    return frames;
}