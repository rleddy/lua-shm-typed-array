#include "../lua_shm.h"

#include <iostream>
#include <unistd.h>
#include <cmath>

using namespace std;


int main(int argc,char **argv) {
	cout << "this is a test" << endl;
	
	key_t mykey = 234234;
	key_t kk = get(mykey, SHMBT_FLOAT64);
	
	cout << kk << endl;
	
	cout << getTotalSize() << endl;
	
	sleep(4);
	
	Mem_resource *mr = MemResrouceFromKey(kk);

	if ( mr != NULL ) {
		double pi = readFloat64(mr,24);
		double pi2 = readFloat64(mr,25);
		cout << "PI: " << pi << "  PI*2 " << pi2 << endl;
	} else {
		cout << "OOPS" << endl;
	}
	
	cout << detach(kk,false) << endl;
}


// ipcs -a
// ipcrm -m 9797
