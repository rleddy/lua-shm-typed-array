

#include "lua_shm.h"

#include <iostream>
#include <map>
#include <vector>
#include <numeric>

using namespace std;


static int 	detachShmSegment(int resId, void* addr, bool force = false, bool onExit = false);


// Detach segment
// Returns count of left attaches or -1 on error
static int detachShmSegment(int resId, void* addr, bool force, bool onExit) {
	int err;
	struct shmid_ds shminf;
	//detach
	err = shmdt(addr);
	if ( err == 0 ) {
		//get stat
		err = shmctl(resId, IPC_STAT, &shminf);
		if (err == 0) {
			//destroy if there are no more attaches or force==true
			if ( force || shminf.shm_nattch == 0 ) {
				err = shmctl(resId, IPC_RMID, 0);
				if (err == 0) {
					return 0; //detached and destroyed
				} else {
					if( !onExit ) {
						return errno;
					}
				}
			} else {
				return shminf.shm_nattch; //detached, but not destroyed
			}
		} else {
			if( !onExit ) {
				return errno;
			}
		}
	} else {
		switch(errno) {
			case EINVAL: // wrong addr
			default:
				if( !onExit ) {
					return errno;
				}
			break;
		}
	}
	return -1;
}



static int _get(key_t key, unsigned int &count, int shmflg, int at_shmflg, size_t type_size, void **ref, int *resOut) {
	
	int err;
	struct shmid_ds shminf;
	size_t size = count * type_size;
	bool isCreate = (size > 0);
	
	int resId = shmget(key, size, shmflg);
	
	if ( resId == -1 ) {
		switch ( errno ) {
			case EEXIST: // already exists
			case EIDRM:  // scheduled for deletion
			case ENOENT: // not exists
				return -1;
			case EINVAL: // should be SHMMIN <= size <= SHMMAX
				return errno;
			default:
				return errno;
		}
	} else {
		//
		if ( !isCreate ) {
			err = shmctl(resId, IPC_STAT, &shminf);
			if ( err == 0 ) {
				size = shminf.shm_segsz;
				count = (size/type_size);
			} else {
				return errno;
			}
		}
		//
		void *res = shmat(resId, NULL, at_shmflg);
		if ( res == (void *)-1 ) {
			return errno;
		}
		*ref = res;
		*resOut = resId;
		//
		return(0);
	}
}



static map<key_t,Mem_resource *> g_all_regions;

static int _detach(key_t key,bool forceDestroy, bool onExit = false);

static int _detach(key_t key,bool forceDestroy, bool onExit) {
	//
	map<key_t,Mem_resource *>::iterator fnd = g_all_regions.find(key);
	if ( fnd != g_all_regions.end() ) {
		//
		Mem_resource *mres = g_all_regions[key];
		//
		if ( mres != NULL ) {
			int resId = mres->resid;
			void *addr = mres->_shared_resource;
			//
			if ( detachShmSegment(resId, addr, forceDestroy, onExit) >= 0 ) {
				delete mres;
				g_all_regions[key] = NULL;
				delete g_all_regions[key];
			}
		}
		//
	}
	//
	return(g_all_regions.size());
}


// detach a single segment
int detach(key_t key,bool forceDestroy) {
	return _detach(key,forceDestroy);
}


// detach all segments currently in use
int detachAll(void) {
	vector<key_t> keys;
	//
	transform(begin(g_all_regions), end(g_all_regions), back_inserter(keys),
			  [](decltype(g_all_regions)::value_type const& pair) {
				  return pair.first;
			  });
	//
	int res = 0;
	for ( key_t k : keys ) {
		if ( _detach(k,false,true) == 0 ) res++;
	}
	return res;
}

unsigned long int getTotalSize(void) {
	//
	vector<size_t> sizes;
	//
	transform(begin(g_all_regions), end(g_all_regions), back_inserter(sizes),
			  [](decltype(g_all_regions)::value_type const& pair) {
				  size_t sz = pair.second->el_sz;
				  size_t cnt = pair.second->count;
				  return(sz*cnt);
			  });
	//
	auto addSize = []( unsigned long int a, unsigned long int b) {
		return(a + b);
	};
	//
	unsigned long int total = accumulate(sizes.begin(), sizes.end(), 0L, addSize);
	return(total);
}



inline key_t _create(unsigned int count, ShmBufferType type, size_t type_size, int key) {
	//
	int resId = 0;
	void *ref = NULL;
	int status = -1;
	if ( key > 0 ) {
		status = _get(key, count, IPC_CREAT|IPC_EXCL|g_perm, 0, type_size, &ref, &resId);
	} else {
		do {
			key = _keyGen();
			status = _get(key, count, IPC_CREAT|IPC_EXCL|g_perm, 0, type_size, &ref, &resId);
		} while( ref == NULL );
	}
	//
	key_t k = key;
	if ( status == 0 ) {
		Mem_resource *ms = NULL;
		if ( key <= 0 ) {
			k = _keyGen();
			ms = new Mem_resource();
			g_all_regions[k] = ms;
		} else {
			ms = new Mem_resource();
			g_all_regions[key] = ms;
		}
		//
		ms->count = count;
		ms->key = k;
		ms->type = type;
		ms->el_sz = type_size;
		ms->_shared_resource = ref;
		ms->resid = resId;
	}
	//
	return(k);
}

//
key_t create_object_region(unsigned int count, size_t type_size, int key) {
	if ( key < 0 ) return(-1);
	else if ( key > 0 ){
		if ( !( (key >= keyMin) && (key <= keyMax) ) ) {
			return -1;
		}
	}
	//
	if ( !(count >= lengthMin && count <= lengthMax) ) {
		return -1;
	}
	if ( !(type_size >= userSizeMin && type_size <= userSizeMax) ) {
		return -1;
	}
	//
	return(_create(count,SHMBT_USER,type_size,key));
}

//
key_t create(unsigned int count, ShmBufferType type, int key) {
	if ( type < 0 || type >= SHMBT_USER ) type = SHMBT_BUFFER;
	if ( key < 0 ) return(-1);
	else if ( key > 0 ){
		if ( !( (key >= keyMin) && (key <= keyMax) ) ) {
			return -1;
		}
	}
	//
	if ( !(count >= lengthMin && count <= lengthMax) ) {
		return -1;
	}
	//
	size_t type_size = getSize1ForShmBufferType(type);
	return(_create(count,type,type_size,key));
}



inline key_t _get_and_set_memres(size_t type_size, int key, ShmBufferType type) {
	unsigned int count = 0;
	int resId = 0;
	void *ref = NULL;
	int status = _get(key,count,0,0,type_size,&ref,&resId);
	//
	if ( status == 0 ) {
		//
		Mem_resource *ms = new Mem_resource();
		g_all_regions[key] = ms;
		//
		//
		ms->count = count;  // set by memsize (has to be found)
		ms->key = key;
		ms->type = type;
		ms->el_sz = type_size;
		ms->_shared_resource = ref;
		ms->resid = resId;
		//
		return(key);
	}
	//
	return(-1);
}




key_t get_object_region(key_t key,size_t type_size) {
	if ( key < 0 ) return(-1);
	else if ( key > 0 ){
		if ( !( (key >= keyMin) && (key <= keyMax) ) ) {
			return -1;
		}
	}
	if ( !(type_size >= userSizeMin && type_size <= userSizeMax) ) {
		return -1;
	}
	//
	map<key_t,Mem_resource *>::iterator fnd = g_all_regions.find(key);
	if ( fnd != g_all_regions.end() ) {
		return(key);
	} else {
		return(_get_and_set_memres(type_size, key, SHMBT_USER));
	}
	return(-1);
}


key_t get(key_t key, ShmBufferType type) {
	if ( key < 0 ) return(-1);
	else if ( key > 0 ){
		if ( !( (key >= keyMin) && (key <= keyMax) ) ) {
			return -1;
		}
	}
	if ( type < 0 || type >= SHMBT_USER ) type = SHMBT_BUFFER;
	//
	map<key_t,Mem_resource *>::iterator fnd = g_all_regions.find(key);
	if ( fnd != g_all_regions.end() ) {
		return(key);
	} else {
		//
		size_t type_size = getSize1ForShmBufferType(type);
		//
		return(_get_and_set_memres(type_size, key, type));
	}
	return(-1);
}




// ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ====
// ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ====

static int gResError = 0;

inline Mem_resource *_MemResrouceFromKey(key_t key) {
	gResError = 0;
	map<key_t,Mem_resource *>::iterator fnd = g_all_regions.find(key);
	if ( fnd != g_all_regions.end() ) {
		Mem_resource *memres = fnd->second;
		if ( memres == NULL ) {
			gResError = -2;
			return NULL;
		}
		return(memres);
	}
	gResError = -1;
	return NULL;
}


//
//
Mem_resource *MemResrouceFromKey(key_t key) {
	return(_MemResrouceFromKey(key));
}

//
//
int MemResourceError(void) {
	return(gResError);
}



//// ---------- // ----------// ----------// ----------// ----------// ----------
// char *
//// ---------- // ----------// ----------// ----------// ----------// ----------


int lshm_write(Mem_resource *memres,unsigned int offset,const char *data,int size) {
	int w_err = 0;
	if ( memres != NULL ) {
		if ( (memres->_shared_resource != NULL) && (size < memres->count) ) {
			char *here = (char *)(memres->_shared_resource) + offset;
			char *end = (char *)(memres->_shared_resource) + memres->count;
			if ( (here + size) < end ) {
				memcpy(here,data,size);
				return(0);
			} else {
				w_err = -4;
			}
		} else {
			w_err = -3;
		}
	} else {
		w_err = gResError;
	}
	gResError = w_err;
	// error
	return(w_err);
}


//
//
int lshm_writeKey(key_t key,unsigned int offset,const char *data,int size) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(lshm_write(memres,offset,data,size));
}


char *lshm_read(Mem_resource *memres,unsigned int offset,int size) {
	if ( memres != NULL ) {
		if ( (memres->_shared_resource != NULL) && (size < memres->count) ) {
			char *here = (char *)(memres->_shared_resource) + offset;
			char *end = (char *)(memres->_shared_resource) + memres->count;
			if ( (here + size) < end ) {
				char *result = new char[size+1];
				result[size] = 0;
				memcpy(result,here,size);
				return(result);
			} else {
				gResError = -4;
			}
		} else {
			gResError = -3;
		}
	}
	return(NULL);
}


char *lshm_readKey(key_t key,unsigned int offset,int size) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(lshm_read(memres,offset,size));
}




//// ---------- // ----------// ----------// ----------// ----------// ----------
//// ---------- // ----------// ----------// ----------// ----------// ----------
//// ---------- // ----------// ----------// ----------// ----------// ----------
//// ---------- // ----------// ----------// ----------// ----------// ----------


template<typename T>
inline int _write(Mem_resource *memres,unsigned int index,T word) {
	int w_err = 0;
	void *res = memres->_shared_resource;
	if ( (res != NULL) && (index < memres->count) ) {
		T *here;
		if ( memres->type == 0 ) {
			here = (T *)(((char *)res) + index);
		} else {
			here = (T *)(res) + index;
		}
		*here = word;
		return(0);
	} else {
		w_err = -3;
	}
	gResError = w_err;
	return(w_err);
}


template<typename T>
inline T _read(Mem_resource *memres,unsigned int index) {
	gResError = 0;
	void *res = memres->_shared_resource;
	if ( (res != NULL) && (index < memres->count) ) {
		if ( memres->type == 0 ) {
			T word = *((T *)(((char *)res) + index));
			return(word);
		} else {
			T word = *((T *)(res) + index);
			return(word);
		}
	} else {
		gResError = -3;
	}
	throw gResError;
}

//// ---------- // ----------// ----------// ----------// ----------// ----------
// Int8
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeInt8(Mem_resource *memres,unsigned int index,char word) {
	if ( memres != NULL ) {
		return(_write<char>(memres,index,word));
	}
	return(gResError); // error
}
//
int writeUInt8Key(key_t key,unsigned int index,char word) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(writeUInt8(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----

char readInt8(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<char>(memres,index));
	}
	return(( char)(-1)); // error
}
//
char readInt8Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readUInt8(memres,index));
}

//// ---------- // ----------// ----------// ----------// ----------// ----------
// UInt8
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeUInt8(Mem_resource *memres,unsigned int index,unsigned char word) {
	if ( memres != NULL ) {
		return(_write<unsigned char>(memres,index,word));
	}
	return(gResError); // error
}

int writeUInt8Key(key_t key,unsigned int index,unsigned char word) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(writeUInt8(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----

unsigned char readUInt8(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<unsigned char>(memres,index));
	}
	return((unsigned char)(-1)); // error
}


//
unsigned char readUInt8Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readUInt16(memres,index));
}


//// ---------- // ----------// ----------// ----------// ----------// ----------
// Int16
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeInt16(Mem_resource *memres,unsigned int index,short int word) {
	if ( memres != NULL ) {
		return(_write<short int>(memres,index,word));
	}
	return(gResError); // error
}

int writeInt16Key(key_t key,unsigned int index,short int word) {
	Mem_resource *memres = _MemResrouceFromKey(key);;
	return(writeUInt16(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
short int readInt16(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<short int>(memres,index));
	}
	return((short)(-1)); // error
}

//
short int readInt16Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readUInt16(memres,index));
}


//// ---------- // ----------// ----------// ----------// ----------// ----------
// UInt16
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeUInt16(Mem_resource *memres,unsigned int index,unsigned short int word) {
	if ( memres != NULL ) {
		return(_write<unsigned short int>(memres,index,word));
	}
	return(gResError); // error
}

int writeUInt16Key(key_t key,unsigned int index,unsigned short int word) {
	Mem_resource *memres = _MemResrouceFromKey(key);;
	return(writeUInt16(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
unsigned short int readUInt16(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<unsigned short int>(memres,index));
	}
	return((short)(-1)); // error
}

//
unsigned short int readUInt16Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readUInt16(memres,index));
}


//// ---------- // ----------// ----------// ----------// ----------// ----------
// Int32
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeInt32(Mem_resource *memres,unsigned int index,int word) {
	if ( memres != NULL ) {
		return(_write<int>(memres,index,word));
	}
	return(gResError); // error
}

int writeInt32Key(key_t key,unsigned int index,int word) {
	Mem_resource *memres = _MemResrouceFromKey(key);;
	return(writeUInt32(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
int readInt32(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<int>(memres,index));
	}
	return((short)(-1)); // error
}

//
int readInt32Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readInt32(memres,index));
}


//// ---------- // ----------// ----------// ----------// ----------// ----------
// UInt32
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeUInt32(Mem_resource *memres,unsigned int index,unsigned int word) {
	if ( memres != NULL ) {
		return(_write<unsigned int>(memres,index,word));
	}
	return(gResError); // error
}

int writeUInt32Key(key_t key,unsigned int index,unsigned int word) {
	Mem_resource *memres = _MemResrouceFromKey(key);;
	return(writeUInt32(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
unsigned int readUInt32(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<unsigned int>(memres,index));
	}
	return((short)(-1)); // error
}

//
unsigned int readUInt32Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readUInt32(memres,index));
}



//// ---------- // ----------// ----------// ----------// ----------// ----------
// Float32
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeFloat32(Mem_resource *memres,unsigned int index,float word) {
	if ( memres != NULL ) {
		return(_write<float>(memres,index,word));
	}
	return(gResError); // error
}

int writeFloat32Key(key_t key,unsigned int index,float word) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(writeFloat32(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
float readFloat32(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<float>(memres,index));
	}
	return((short)(-1)); // error
}

//
float readFloat32Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readFloat32(memres,index));
}


//// ---------- // ----------// ----------// ----------// ----------// ----------
// Float64
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeFloat64(Mem_resource *memres,unsigned int index,double word) {
	if ( memres != NULL ) {
		return(_write<double>(memres,index,word));
	}
	return(gResError); // error
}

int writeFloat64Key(key_t key,unsigned int index,double word) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(writeFloat64(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
double readFloat64(Mem_resource *memres,unsigned int index) {
	if ( memres != NULL ) {
		return(_read<double>(memres,index));
	}
	return((short)(-1)); // error
}

//
double readFloat64Key(key_t key,unsigned int index) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readFloat64(memres,index));
}



//// ---------- // ----------// ----------// ----------// ----------// ----------
// Complex64
//// ---------- // ----------// ----------// ----------// ----------// ----------

int writeComplex64(Mem_resource *memres,unsigned int index,complex<double> word) {
	if ( memres != NULL ) {
		return(_write<complex<double>>(memres,index,word));
	}
	return(gResError); // error
}

int writeComplex64Key(key_t key,unsigned int index,complex<double> word) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(writeComplex64(memres,index,word));
}

// ---- ---- ---- ---- ---- ---- ---- ---- ----
//
complex<double> *readComplex64(Mem_resource *memres,unsigned int index,complex<double> *appCmplx) {
	if ( memres != NULL ) {
		*appCmplx = _read<complex<double>>(memres,index);
	} else {
		return(NULL);
	}
	return(appCmplx);
}

//
complex<double> *readComplex64Key(key_t key,unsigned int index,complex<double> *appCmplx) {
	Mem_resource *memres = _MemResrouceFromKey(key);
	return(readComplex64(memres,index,appCmplx));
}


