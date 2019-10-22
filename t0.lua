--[[
		try out structures
--]]

local lib = {}

function lib:checkTime()
	local rsize = 11000000    --11000000
	print(rsize)
	local x = os.clock()
	local s = 0
	for i=1,rsize do s = s + i end
	print(string.format("elapsed time: %.2f\n", os.clock() - x))
	print(collectgarbage("count"))
end

--return lib
lib:checkTime()
