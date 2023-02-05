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
#define lineLength 1536
#define BUFF_SIZE 2048
#define SWS 1
#define INTERVAL 150
#define INFINITE 1e9


pthread_t tid;

int cwnd = SWS;
int ssthresh = INFINITE;
int maxSWS = -INFINITE;

int sockSD;
char buffer[BUFF_SIZE];
struct sockaddr_in servAddr;
vector<string> frames;
vector<int> unReceivedAcks;
bool lastAckReceived = false;

int nTimeout = 0;

int lar = 0, lfs = 0;

int FrameContent(char buffer[]);

string readFile(string fileName);

vector<string> makeFrame(int frameSize, string input);

void* recieveAck(void* args);

void timeoutHandle(int a);

int main() {

    if ((sockSD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;


    string input = readFile("test.txt");
    frames = makeFrame(lineLength, input);



    auto start = high_resolution_clock::now();

    string str = to_string(frames.size());
    char num_char[str.length()];
    strcpy(num_char, str.c_str());

    sendto(sockSD, num_char, BUFF_SIZE+1,
           MSG_CONFIRM, (const struct sockaddr *) &servAddr,
           sizeof(servAddr));

    pthread_create(&tid, nullptr, recieveAck, nullptr);


    while(!lastAckReceived) {
        while(lfs >= frames.size())
            continue;
        if(lfs-lar < cwnd) {
            string SNS = to_string(lfs+1);

            int n = SNS.length() + frames[lfs].length() + 1;

            n += to_string(n).length();
            char char_array[n+1];
            strcpy(char_array, (SNS + " " + frames[lfs]).c_str());

            sendto(sockSD, char_array, BUFF_SIZE+1,
                   MSG_CONFIRM, (const struct sockaddr *) &servAddr,
                   sizeof(servAddr));

            lfs++;
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    pthread_join(tid, nullptr);
    cout << "Duration of transmission for a 1MB file test.txt: " << duration.count() / 1000000 << " seconds" << endl;
    close(sockSD);
    return 0;
}

int FrameContent(char buffer[]) {
    int seqN;
    string temp = "";

    int i = 0;
    while (true) {
        if (buffer[i] == ' ')
            break;

        temp += buffer[i];
        i++;
    }
    seqN = stoi(temp);
    return seqN;
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
    while(lar < frames.size()) {
        struct itimerval it_val;
        it_val.it_value.tv_sec = INTERVAL/1000;
        it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;
        it_val.it_interval = it_val.it_value;
        memset(&buffer, 0, sizeof(buffer));
        signal(SIGALRM, timeoutHandle);
//alarm(TIME_OUT_DUR);
        setitimer(ITIMER_REAL, &it_val, NULL);
        recvfrom(sockSD, (char *)buffer, BUFF_SIZE+1,
                 MSG_WAITALL, ( struct sockaddr *) &servAddr,
                 (socklen_t*)&servAddr);
        int SN = FrameContent(buffer);
        cout << "ack received for packet " << atoi(buffer) << endl << endl;
        cerr << lar << " " << SN << endl;
        if(lar+1 == SN) {
            cout << "sliding the window to the right...\n";
            sort(unReceivedAcks.begin(), unReceivedAcks.end(), greater <>());
            lar++;
            for(int i = unReceivedAcks.size()-1; i >= 0; i--)
                if(lar+1 == unReceivedAcks[i]) {
                    lar++;
                    unReceivedAcks.pop_back();
                }
        } else {
            unReceivedAcks.push_back(SN);
        }
    }
    char *transmitComplete = (char*)(to_string(frames.size()+1).c_str());
    sendto(sockSD, transmitComplete, BUFF_SIZE+1,
           MSG_CONFIRM, (const struct sockaddr *) &servAddr,
           sizeof(servAddr));
    return nullptr;
}

void timeoutHandle(int a) {
    cout << "Time Out Occured!\n\n";
    nTimeout++;
    if(nTimeout >= 1) {
        ssthresh = cwnd / 2;
        cwnd = 1;
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
