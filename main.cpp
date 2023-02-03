#include <iostream>
#include <thread>
#include <vector>
using namespace std;

#define NUM_OF_HOST 20
#define NUM_OF_OPS 22
pthread_t tid[NUM_OF_OPS];
vector<int> CNS(NUM_OF_HOST, 0);

void* exeA(void* i) {
    int CN = *((int*) i);
    cout << CN << endl;
    system(("./a.out " + to_string(CN+1) + " > a1"+to_string(CN+1)+".txt").c_str());
    return nullptr;
}

void* exeB(void* args) {
    system("./b.out > bـ.txt");
    return nullptr;
}

void* exeR(void* args) {
    system("./r.out > rـ.txt");
    return nullptr;
}

int main() {

    pthread_create(&tid[0], nullptr, exeB, nullptr);
    pthread_create(&tid[1], nullptr, exeR, nullptr);
    for(int i = 0; i < NUM_OF_HOST; i++) {
        CNS[i] = i;
        pthread_create(&tid[i + 2], nullptr, exeA, (void *) &(CNS[i]));
    }
    for(int i = 0;
        i < NUM_OF_OPS; i++)
        pthread_join(tid[i], nullptr);

}