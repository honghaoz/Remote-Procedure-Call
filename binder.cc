/************************
CS454 A3 binder.cc
created by Chao


*************************/


#include <iostream>
using namespace std;

typedef int (*skeleton)(int, int);

int add(int a, int b) {
    return a + b;
}



int main(int argc, const char * argv[])
{
    skeleton aa = &add;
    (*aa)(123, 321);
    cout << "Hello, World!\n";
    return 0;
}