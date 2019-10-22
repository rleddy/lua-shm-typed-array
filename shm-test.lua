local ffi = require("ffi")
local lshm = require("lua-shm-type-array/shm_typed_array")

local shmkey = 3453
local buf = lshm.create(4096,'SHMBT_BUFFER',shmkey)

lshm.lshm_writeKey(shmkey,10,"you are here",10);

local memres = lshm.MemResrouceFromKey(shmkey);
lshm.lshm_write(memres,20,"you are here",10);

local rsize = 11000000    --11000000

local x = os.clock()

for i=1,rsize do
	lshm.lshm_write(memres,0,"you are here",10);
end

print(string.format("elapsed time: %.2f\n", os.clock() - x))
print(collectgarbage("count"))

lshm.detach(shmkey,true)
