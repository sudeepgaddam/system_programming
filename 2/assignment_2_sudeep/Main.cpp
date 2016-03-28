#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <vector>
#include <queue>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "fstream"
#include <algorithm>
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds


using namespace std;


#define MAPPER_POOL_SIZE 5
#define REDUCER_POOL_SIZE 5
#define SUMMARISER_POOL_SIZE 5



/*
 * Mapper pool, Writtern by mapper pool updater, Read by mapper thread
 * Reducer pool, written by mapper thread, Read by reducer
 * Summariser_pool, Written by reducer thread, read by summeriser and word count writer
 * Letter_count_table, Written by summariser, Read by Letter_count_writer_thread
 * 
 * 
 * Mapper pool updater and word count writer thread
 */
queue < vector<string> > mapper_pool;
queue < vector<string> > reducer_pool;
queue < vector<string> > summariser_pool;
vector<int> letter_count_table(26,0);

pthread_mutex_t mapper_pool_mutex,reducer_pool_mutex,summariser_pool_mutex;	
pthread_cond_t mapper_pool_empty,mapper_pool_full, reducer_pool_empty, reducer_pool_full,summariser_pool_full, summariser_pool_empty;

/*
 * Name: mapper_pool_update, Thread handler
 * 
 * Input: Input File name
 * Description:
 * Reads words from the input file, writes words to different entries depending 
 * on the starting character
 * 
 * Writes words to mapper_pool
 */
void *mapper_pool_update(void *argv) {
	ifstream file;
    file.open ((char *)argv);
    string word;
    int i = 1;
	//cout<<"mapper_pool_update"<<endl;
    vector<string> words;
    while (file >> word)
    {	
		words.push_back(word);
	}
	i=0;
	while(1) {
		
		pthread_mutex_lock(&mapper_pool_mutex);
		int prev = i;
		if (mapper_pool.size() > MAPPER_POOL_SIZE)  {
				cout<< " Greater than Mapper pool size, Cant write, Waiting..."<<endl;
				pthread_cond_wait(&mapper_pool_full,	
							&mapper_pool_mutex);
		}
		while((i<(words.size()-1)) && (words[i][0] == words[i+1][0])) {
			i++;
		}
		if(i>= words.size()) {
			pthread_mutex_unlock(&mapper_pool_mutex);
			return 0;
		} else if(i == (words.size()-1)) {
			mapper_pool.push(vector<string>(words.begin()+prev,words.begin()+i+1));
			pthread_cond_signal(&mapper_pool_empty);
			pthread_mutex_unlock(&mapper_pool_mutex);
			return 0;
		} else {
			mapper_pool.push(vector<string>(words.begin()+prev,words.begin()+i+1));
			pthread_cond_signal(&mapper_pool_empty);
			pthread_mutex_unlock(&mapper_pool_mutex);
		}
		
		i++;

	}
}
/*
 * Name: write_to_reducer_pool
 * 
 * 
 * Description:
 * Given a row of words, writes them to reducer pool
 * 
 * Writes word,1 to reducer_pool
 */
void write_to_reducer_pool(vector<string> & words){
	vector<string> mapper_words;
	for(auto word: words) {
		mapper_words.push_back("(" + word+ ",1)");
	}
	
	pthread_mutex_lock(&reducer_pool_mutex);

	if (reducer_pool.size() > REDUCER_POOL_SIZE)  {
		pthread_cond_wait(&reducer_pool_full,	
	                      &reducer_pool_mutex);
	}
	cout<<"write_to_reducer_pool"<<endl;
	reducer_pool.push(mapper_words);
	pthread_cond_signal(&reducer_pool_empty);
	pthread_mutex_unlock(&reducer_pool_mutex);

	
}

/*
 * Name: mapper, Thread handler
 * 
 * 
 * Description:
 * Thread handler of mapper
 * Reads word counts from mapper pool, Gets an entire row from mapper_pool
 * Writes word,1 to reducer_pool
 */
void *mapper(void *mapper_data) {
	//cout<<"mapper"<<endl;
	vector<string> words;
	while(1) {
		pthread_mutex_lock(&mapper_pool_mutex);
		
		
		if (mapper_pool.empty())  {
			cout<< " Mapper pool Empty, Cant read, Waiting..."<<endl;
			pthread_cond_wait(&mapper_pool_empty,	
							  &mapper_pool_mutex);
		}
		if(!mapper_pool.empty()) {	
			words = mapper_pool.front();
			mapper_pool.pop();
		}
		pthread_cond_signal(&mapper_pool_full);
		pthread_mutex_unlock(&mapper_pool_mutex);
		write_to_reducer_pool(words);
	}
}

/*
 *	Name: write_to_summariser
 * 	I/p:  vector<string> &words
 * 
 * 	Description: writes to summariser pool
 *  words are of form (word,1). All beginning with same character 
 * 
 * 
 * Option 1: Sort and then O(n) loop for getting word counts
 * Option 2: O(n) loop and have O(n) space map
 * 
 */
void write_to_summariser(vector<string> &words){
	vector<string> reducer_words;
	cout<<"Writing to Summariser" << endl;
	int size = words.size();

	
	sort(words.begin(), words.end());
	
	int count = 1;
	
	for(int i=1;i<size;i++) {
		if(words[i] != words[i-1]) {
			//If prev word != current word, push the prev word

			std::size_t pos = words[i-1].find(",");      
			std::string str3 = words[i-1].substr (1,pos-1);
			reducer_words.push_back("("+str3+","+to_string(count)+")");
			count = 1;
		} else {
			//If prev word == current word, increment count
			count++;
		}
		//Push the Last word
		if (i==(size-1)) {
			std::size_t pos = words[i].find(",");      
			std::string str3 = words[i].substr (1,pos-1);
			reducer_words.push_back("("+str3+","+to_string(count)+")");
		}
	}

	pthread_mutex_lock(&summariser_pool_mutex);

	if (summariser_pool.size() > SUMMARISER_POOL_SIZE)  {
		pthread_cond_wait(&summariser_pool_full,	
	                      &summariser_pool_mutex);
	} 
	summariser_pool.push(reducer_words);
	pthread_cond_signal(&summariser_pool_empty);
	pthread_mutex_unlock(&summariser_pool_mutex);
}
/*
 * Name: reducer, Thread handler
 * 
 * 
 * Description:
 * Thread handler of reducer
 * Reads word counts from reducer_pool,
 * Writes summarised word counts to summariser_pool
 */

void *reducer(void *reducer_data) {
	vector<string> words;
	while(1) {
		pthread_mutex_lock(&reducer_pool_mutex);
		if (reducer_pool.empty())  {
			cout<< " Reducer pool Empty, Cant read, Waiting..."<<endl;
			pthread_cond_wait(&reducer_pool_empty,	
							  &reducer_pool_mutex);
		}
		if(!reducer_pool.empty()) {
			words = reducer_pool.front();
			reducer_pool.pop();
		}
		pthread_cond_signal(&reducer_pool_full);
		pthread_mutex_unlock(&reducer_pool_mutex);
		write_to_summariser(words);
	
	}
}
/*
 * Name: write_from_summariser_to_file
 * 
 * 
 * Description:
 * Reads word counts from summariser pool,
 * Writes wordcount count to wordCount.txt
 */

void write_from_summariser_to_file(vector<string> & words, ofstream &file){
	file.open ("wordCount.txt",std::ofstream::out | std::ofstream::app);
	for(auto a: words){
		//cout<<a<<",";
		 file<<a<<endl;
	 }
	 //cout<<endl;
	file.close();
}
/*
 * Name: word_count_writer, Thread handler
 * 
 * 
 * Description:
 * Thread handler of word_count_writer
 * Reads word counts from summariser pool,
 * Writes letter count to wordCount.txt
 */

void *word_count_writer(void *summariser_data) {
	ofstream file;
	file.open ("wordCount.txt");
	file.close();
	while(1) {
		pthread_mutex_lock(&summariser_pool_mutex);
		if (summariser_pool.empty())  {
			pthread_cond_wait(&summariser_pool_empty,	
							  &summariser_pool_mutex);
		} 
		vector<string> word_counts = summariser_pool.front();
	
		summariser_pool.pop();
		pthread_cond_signal(&summariser_pool_full);
		pthread_mutex_unlock(&summariser_pool_mutex);
		write_from_summariser_to_file(word_counts,file);

	}
	
		
}
/*
 * Name: write_to_letter_table
 * 
 * 
 * Description:
 * Reads word counts from summariser pool,
 * Writes letter count to letter_count_table
 */

void  write_to_letter_table(queue<vector <string> > &words) {
	while(!words.empty()) {
		
		auto a = words.front();
		cout<<a[0]<<endl;
		std::size_t pos = a[0].find(",");      
		int count  = (int)a[0][pos+1];

		letter_count_table[a[0][1]-'a']+=count;
		words.pop();
	}
	
}
/*
 * Name: letter_table_update
 * 
 * 
 * Description:
 * Reads word counts from summariser pool,
 * Writes letter count to letter_count_table
 */

void *letter_table_update(void *summariser_data) {
	
	queue<vector<string> > word_counts = summariser_pool;
	write_to_letter_table(word_counts);

	
}




/*
 * Input File name, Mapper threads, Reducer, Summmarizer threads
 * O/p: Wordcount.txt letterCount.txt
 * 
 * Description:
 *  Mapper pool updater and word count writer thread
 */

int main(int argc, char *argv[]){
	pid_t mapper_id, reducer_id;
	
	if(argc<4){
		cout<<"Give three arguement i.e., Input file, Mapper threads, reducer threads Exiting..." << endl;
		exit(1);
	}

	
	int num_mappers = atoi(argv[2]);
	int num_reducers  = atoi(argv[3]);
	//cout<< num_mappers << " " << num_reducers << endl;
	vector<pthread_t> mappers(num_mappers);
	vector<pthread_t> reducers(num_reducers);
	pthread_t mapper_pool_updater;
	pthread_t summariser_pool_updater;
	pthread_t word_count_writer_thread;
	pthread_t letter_table_updater;

	pthread_create (&mapper_pool_updater, NULL, mapper_pool_update,argv[1]);
	//pthread_create (&summariser_pool_updater, NULL, summariser_pool_update,NULL);
	pthread_create (&word_count_writer_thread, NULL, word_count_writer,NULL);
	//pthread_create (&letter_table_updater, NULL, letter_table_update,NULL);



	for(int i=0;i<num_mappers;i++) {
		    pthread_create (&mappers[i], NULL, mapper,NULL);
	}
	
	for(int i=0;i<num_reducers;i++) {
		pthread_create (&reducers[i], NULL, reducer,NULL);

	}
	
	
	(void) pthread_join(mapper_pool_updater, NULL);
	
	//Comment below two lines for large input files, For small files,
	//2 seconds is enough to process and  write word counts to file
	this_thread::sleep_for (std::chrono::seconds(2));
    return 1;
    
    
    for(int i=0;i<num_mappers;i++) {
		    pthread_join(mappers[i], NULL);
	}
	
	for(int i=0;i<num_reducers;i++) {
		pthread_join(reducers[i], NULL);
	}
	
    //(void) pthread_join(summariser_pool_updater, NULL);

	//pthread_create (&ConnectSender, NULL, send_connects, &peerId);
	
	return 0;
}
