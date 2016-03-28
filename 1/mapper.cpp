#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include <iostream>
#include "fstream"
using namespace std;

void readFile(char *argv)
{
    ifstream file;
    file.open (argv);
    string word;

    while ( file >> word)
    {
            cout<<"(" << word << ",1)" << endl;
    }
}


int main(int argc, char *argv[]) {
	readFile(argv[1]);
}
