#include "../lua_shm.h"

#include <iostream>
#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <cmath>

using namespace std;


static bool gRunning = true;

void stopRunning(int sig) {
	gRunning = false;
}


int main(int argc,char **argv) {
	cout << "this is a test" << endl;
	
	signal(SIGINT,stopRunning);
	
	key_t mykey = 234234;
	//key_t kk = create(100, SHMBT_FLOAT64, mykey);   SHMBT_BUFFER
	key_t kk = create(200*sizeof(double), SHMBT_BUFFER, mykey);
	
	cout << kk << endl;
	
	cout << getTotalSize() << endl;
	
	Mem_resource *mr = MemResrouceFromKey(kk);

	double pi = acos(-1);
	
	cout << "PI" << endl;
	
	while ( gRunning ) {
		writeFloat64(mr,24*sizeof(double),pi);
		writeFloat64(mr,25*sizeof(double),2*pi);
		sleep(1);
	}
	
	cout << "detaching" << endl;
	cout << detach(kk,true) << endl;
}


