#ifndef _LUA_SHM_
#define _LUA_SHM_


#include "errno.h"

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <cmath>
#include <complex>
//
#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <limits.h>

using namespace std;

#define SAFE_DELETE(a) if( (a) != NULL ) delete (a); (a) = NULL;
#define SAFE_DELETE_ARR(a) if( (a) != NULL ) delete [] (a); (a) = NULL;


#define NODE_BUFFER_MAX_LENGTH  20000


const unsigned int keyMin = 1;
const unsigned int keyMax = UINT_MAX - keyMin - 1;
const unsigned int g_perm = 0660;
const unsigned int lengthMin = 1;

#define USER_MAX_OBJECT_SIZE 512		// DPS might use 8912
#define USER_MIN_OBJECT_SIZE 12			// bigger than a double

const unsigned int userSizeMax = USER_MAX_OBJECT_SIZE;
const unsigned int userSizeMin = USER_MIN_OBJECT_SIZE;


/**
 * Max length of shared memory segment (count of elements, not bytes)
 */
const unsigned long int lengthMax = NODE_BUFFER_MAX_LENGTH;
void cleanup(void);


static bool g_rand_initialized = false;
static inline void init_rand(void) {
	if ( !g_rand_initialized ) {
		g_rand_initialized = true;
		srand(time(NULL));
	}
}

inline unsigned int _keyGen() {
	init_rand();
	unsigned int rr = (unsigned int)(rand())/100;
	return (keyMin + 100 + rr);
}


//
//

typedef enum {
		SHMBT_BUFFER = 0, //for using Buffer instead of TypedArray
		SHMBT_INT8,
		SHMBT_UINT8,
		SHMBT_UINT8CLAMPED,
		SHMBT_INT16,
		SHMBT_UINT16,
		SHMBT_INT32,
		SHMBT_UINT32,
		SHMBT_FLOAT32,
		SHMBT_FLOAT64,
		SHMBT_COMPLEX64,
		SHMBT_USER
	} ShmBufferType;

//
//
inline size_t getSize1ForShmBufferType(ShmBufferType type) {
	size_t size1 = 0;
	switch(type) {
		case SHMBT_BUFFER:
		case SHMBT_INT8:
		case SHMBT_UINT8:
		case SHMBT_UINT8CLAMPED:
			size1 = 1;
			break;
		case SHMBT_INT16:
		case SHMBT_UINT16:
			size1 = 2;
			break;
		case SHMBT_INT32:
		case SHMBT_UINT32:
		case SHMBT_FLOAT32:
			size1 = 4;
			break;
		case SHMBT_COMPLEX64:
			size1 = 16;
			break;
		case SHMBT_FLOAT64:
		default:
			size1 = 8;
			break;
	}
	return size1;
}


//
typedef struct {
	ShmBufferType	type;
	void 			*_shared_resource;
	unsigned int 	count;
	key_t 			key;
	size_t			el_sz;
	int 			resid;
} Mem_resource;



extern "C" {
	
	/**
	 * Destroy shared memory segment
	 * Params:
	 *  key_t key
	 *  bool force - true to destroy even there are other processed uses this segment
	 * Returns count of left attaches or -1 on error
	 */
	int detach(key_t key,bool forceDestroy);
	
	/**
	 * Detach all created and getted shared memory segments
	 * Returns count of destroyed segments
	 */
	int detachAll(void);
	
	/**
	 * Get total size of all shared segments in bytes
	 */
	unsigned long int getTotalSize(void);
	
	/**
	 * Create shared memory segment
	 * @param {int} count - number of elements
	 * @param {string} typeKey - see keys of BufferType
	 * @param {int/null} key - integer key of shared memory segment, or null to autogenerate
	 * @return {mixed/null} shared memory buffer/array object, or null on error
	 *  Class depends on param typeKey: Buffer or descendant of TypedArray
	 *  Return object has property 'key' - integer key of created shared memory segment
	 */
	key_t create(unsigned int count, ShmBufferType type, int key);
	
	key_t create_object_region(unsigned int count, size_t type_size, int key);
	key_t get_object_region(key_t key,size_t type_size);

	/**
	 * Get shared memory segment
	 * @param {int} key - integer key of shared memory segment
	 * @param {string} typeKey - see keys of BufferType
	 * @return {mixed/null} shared memory buffer/array object, see create(), or null on error
	 */
	key_t get(key_t key, ShmBufferType type);
	//
	Mem_resource *MemResrouceFromKey(key_t key);
	//
	int lshm_write(Mem_resource *memres,unsigned int offset,const char *data,int size);
	int lshm_writeKey(key_t key,unsigned int offset,const char *data,int size);
	char *lshm_read(Mem_resource *memres,unsigned int offset,int size);
	char *lshm_readKey(key_t key,unsigned int offset,int size);
	// Int8
	int writeInt8(Mem_resource *memres,unsigned int index,char word);
	int writeInt8Key(key_t key,unsigned int index,char word);
	char readInt8(Mem_resource *memres,unsigned int index);
	char readInt8Key(key_t key,unsigned int index);
	// Uint8
	int writeUInt8(Mem_resource *memres,unsigned int index,unsigned char word);
	int writeUInt8Key(key_t key,unsigned int index,unsigned char word);
	unsigned char readUInt8(Mem_resource *memres,unsigned int index);
	unsigned char readUInt8Key(key_t key,unsigned int index);
	// Int16
	int writeInt16(Mem_resource *res,unsigned int index,short int word);
	int writeInt16Key(key_t key,unsigned int index,short int word);
	short int readInt16(Mem_resource *memres,unsigned int index);
	short int readInt16Key(key_t key,unsigned int index);
	// UInt16
	int writeUInt16(Mem_resource *res,unsigned int index,unsigned short int word);
	int writeUInt16Key(key_t key,unsigned int index,unsigned short int word);
	unsigned short int readUInt16(Mem_resource *memres,unsigned int index);
	unsigned short int readUInt16Key(key_t key,unsigned int index);
	// Int32
	int writeInt32(Mem_resource *res,unsigned int index,int word);
	int writeInt32Key(key_t key,unsigned int index,int word);
	int readInt32(Mem_resource *memres,unsigned int index);
	int readInt32Key(key_t key,unsigned int index);
	// UInt32
	int writeUInt32(Mem_resource *res,unsigned int index,unsigned int word);
	int writeUInt32Key(key_t key,unsigned int index,unsigned int word);
	unsigned int readUInt32(Mem_resource *memres,unsigned int index);
	unsigned int readUInt32Key(key_t key,unsigned int index);
	// Float32
	int writeFloat32(Mem_resource *res,unsigned int index,float word);
	int writeFloat32Key(key_t key,unsigned int index,float word);
	float readFloat32(Mem_resource *memres,unsigned int index);
	float readFloat32Key(key_t key,unsigned int index);
	// Float64
	int writeFloat64(Mem_resource *res,unsigned int index,double word);
	int writeFloat64Key(key_t key,unsigned int index,double word);
	double readFloat64(Mem_resource *memres,unsigned int index);
	double readFloat64Key(key_t key,unsigned int index);
	//
	// Complex64
	int writeComplex64(Mem_resource *res,unsigned int index,complex<double> word);
	int writeComplex64Key(key_t key,unsigned int index,complex<double> word);
	complex<double> *readComplex64(Mem_resource *memres,unsigned int index,complex<double> *appCmplx);
	complex<double> *readComplex64Key(key_t key,unsigned int index,complex<double> *appCmplx);
}

/**
 * Create or get shared memory
 * Params:
 *  key_t key
 *  size_t count - count of elements, not bytes
 *  int shmflg - flags for shmget()
 *  int at_shmflg - flags for shmat()
 *  enum ShmBufferType type
 * Returns buffer or typed array, depends on input param type
 */

//void *_get(key_t key, unsigned int count, ShmBufferType type;

#endif


