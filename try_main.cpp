

#include "NonBlockJob.h"

#include <stdio.h>
#include <iostream>
#include <string.h>

// ________________________________TRY MAIN____________________________________________

int parseLineForThreadCount(char* line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    i = atoi(p);
    return i;
}


bool GetThreadsStatus(int& threadsCount, int& maxPossibleThreads){
	FILE* file = fopen("/proc/self/status", "r");
	char line[128];
	bool returnValue = false;
	if (file){
		while (fgets(line, 128, file) != NULL){
			if (strncmp(line, "Threads:", 8) == 0){
				threadsCount = parseLineForThreadCount(line);

				FILE* fileMaxThreads = fopen("/proc/sys/kernel/threads-max", "r");
				if(fileMaxThreads){
					if( fgets(line, 128, fileMaxThreads) != NULL){
						maxPossibleThreads = atoi(line);
					}
					fclose(fileMaxThreads);
				}
				returnValue = true;
				break;
			}
		}
		fclose(file);

	}
	else {
		printf("Failed to get thread statistics: %s",strerror(errno));
	}
	return returnValue;
}


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
// 		/*auto fut = */NonBlockJob::getInstance().try_run_for(2000, [this, &data](){
// 							  m_dorWrapper->d_read(data);
// 						    });
// // 		std::cout << "fut: " << fut.get() << std::endl;
// 		// if the thread didn't set yet the promise value, main thread will wait here
// 	}
// 	catch (const std::exception& e) {
// 		std::cout << "Ran catch : " << e.what() << std::endl;
// 	}
	
	int threadsCount = 0, maxThreads = 0;
 	constexpr int NUM_OF_THREADS = 4;
 	std::array<std::thread, NUM_OF_THREADS> threads;

	for (int i = 0; i < NUM_OF_THREADS; ++i) {

		threads[i] = std::thread([=]() {	

			try {		
				/*auto fut = */NonBlockJob::getInstance().try_run_for(6000, [this, &data](){
									  m_dorWrapper->d_read(data);
								    });
		// 		std::cout << "fut: " << fut.get() << std::endl;
				// if the thread didn't set yet the promise value, main thread will wait here
			}
			catch (const std::exception& e) {
				std::cout << "Ran catch : " << e.what() << std::endl;
			}
			printf("\n");
		  
  
 		});
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if( GetThreadsStatus(threadsCount,maxThreads) ){
		std::cout << "Number of threads in process [" << threadsCount << "/" << maxThreads << "]\n";
		}
		printf("\n");
		
	}	
      
      
	//wait for all
	for (auto& t : threads) {
		t.join();
		if( GetThreadsStatus(threadsCount,maxThreads) ){
		std::cout << "Number of threads in process [" << threadsCount << "/" << maxThreads << "]\n";
		}
		printf("\n");
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





