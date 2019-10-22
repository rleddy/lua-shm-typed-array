--[[
		try out structures
--]]

local ffi = require("ffi")
ffi.cdef[[
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
	typedef struct Mem_resource Mem_resource;
	typedef int key_t;
	//
	int detach(key_t key,bool forceDestroy);
	void detachAll(void);
	unsigned long int getTotalSize(void);
	key_t create(unsigned int count, ShmBufferType type, int key);
	key_t get_object_region(key_t key,size_t type_size);
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

	//	//
]]


local lshm = ffi.load("luashm")
--
return lshm
