#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include <iostream>
#include "fstream"
#include <map>
using namespace std;

void readInput()
{
	string word;
	 std::map< std::string,int> mylist;
	
	//int i;
    while (!cin.eof())
    {		
            cin>>word;
            std::size_t pos = word.find(",");      
			std::string str3 = word.substr (1,pos-1);
            if(mylist.empty()) {
				mylist[str3] = 1;
				
			} else if(((mylist.begin()))->first[0] != str3[0]) {

				for(auto it=mylist.begin(); it!=mylist.end(); ++it){
					std::cout<<"(" << (it)->first << "," << (it)->second <<")"<<endl;	
				}
				mylist.clear();
				mylist[str3] = 1;
				
			} else {
				if(mylist.find(str3) != mylist.end()) {
					mylist[str3]++;
				} else {
					mylist[str3] = 1;
				}
			}
			 
	}
    
    
    for (auto it=mylist.begin(); it!=mylist.end(); ++it){
		std::cout<<"(" << (it)->first << "," << (it)->second <<")"<<endl;	
	}
	mylist.clear();


}
int main() {
	readInput();
}
