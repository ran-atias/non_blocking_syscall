
//#include <stdio.h>
#include "NonBlockJob.h"
//#include <unistd.h>
//#include <iostream>

#include <fstream>
#include <string>
#include <sstream>
#include <sys/statvfs.h>
#include <stdio.h>
#include <string.h>


// int parseLineForThreadCount(char* line)
// {
//     // This assumes that a digit will be found and the line ends in " Kb".
//     int i = strlen(line);
//     const char* p = line;
//     while (*p <'0' || *p > '9') p++;
//     i = atoi(p);
//     return i;
// }
// 
// 
// bool GetThreadsStatus(int& threadsCount, int& maxPossibleThreads){
// 	FILE* file = fopen("/proc/self/status", "r");
// 	char line[128];
// 	bool returnValue = false;
// 	if (file){
// 		while (fgets(line, 128, file) != NULL){
// 			if (strncmp(line, "Threads:", 8) == 0){
// 				threadsCount = parseLineForThreadCount(line);
// 
// 				FILE* fileMaxThreads = fopen("/proc/sys/kernel/threads-max", "r");
// 				if(fileMaxThreads){
// 					if( fgets(line, 128, fileMaxThreads) != NULL){
// 						maxPossibleThreads = atoi(line);
// 					}
// 					fclose(fileMaxThreads);
// 				}
// 				returnValue = true;
// 				break;
// 			}
// 		}
// 		fclose(file);
// 
// 	}
// 	else {
// 		printf("Failed to get thread statistics: %s",strerror(errno));
// 	}
// 	return returnValue;
// }


class Dor {

public:

  int d_read(char* data) {
 
    	  printf("before read\n");   
// 	  if (read(0,data,128) < 0) { // o is STDIN
// 	    printf("read faild\n");
// 	    return false;
// 	  } 
	  
	  std::cin >> data;
	   printf("after read :)\n");
	   return 5;
  }
  
  
private:
   
};


class Ran {

public:

    Ran(Dor* dorWrapper) {
     m_dorWrapper = dorWrapper; 
    }
    ~Ran() {
    }
    
    void Act(char* data) {

// 	try {		
// 		auto fut = NonBlockJob::getInstance().try_non_blocking_job_for(std::chrono::milliseconds(2000), [this, &data](){
// 							  return m_dorWrapper->d_read(data);
// 						    });
// 		//std::cout << "fut: " << fut.get() << std::endl;
// 		// if the thread didn't set yet the promise value, main thread will wait here
// 	}
// 	catch (const std::exception& e) {
// 		std::cout << "catch : " << e.what() << std::endl;
// 	}
// 	int threadsCount = 0, maxThreads = 0;
 	constexpr int NUM_OF_THREADS = 4;
 	std::array<std::thread, NUM_OF_THREADS> threads;

	for (int i = 0; i < NUM_OF_THREADS; ++i) {

		threads[i] = std::thread([=]() {	

			  if (	NonBlockJob::getInstance().try_non_blocking_void_job_for(std::chrono::milliseconds(6000), [this, &data](){
									      //std::this_thread::sleep_for(std::chrono::seconds(100));
									      m_dorWrapper->d_read(data);
									})  ) 
			  {
			      printf("return true\n");
			  }
			  else {
			    printf("return false\n");
			  }

		  
		  
 		});
		std::this_thread::sleep_for(std::chrono::seconds(1));
		printf("\n");
// 		if( GetThreadsStatus(threadsCount,maxThreads) ){
// 		std::cout << "Number of threads in process [" << threadsCount << "/" << maxThreads << "]\n";
// 		}
		
	}	
      
      
	//wait for all
	for (auto& t : threads) {
		t.join();
// 		if( GetThreadsStatus(threadsCount,maxThreads) ){
// 		std::cout << "Number of threads in process [" << threadsCount << "/" << maxThreads << "]\n";
// 		}	
	}
      
      
      
    }
    
    
private:
    Dor* m_dorWrapper;
  
};


int main(int argc, char *argv[])
{
  
  char data[128] = {0};
  Dor* ldor = new Dor();
 
  Ran ran(ldor);
  
  ran.Act(data);
  
  printf("echo : [%s]\n" , data);
  
  delete ldor;
  
  return 0;
}























