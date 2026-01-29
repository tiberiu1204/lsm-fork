local jit = require("jit")
local bit = require("bit")
jit.opt.start("hotloop=1")

local result = 0
for i = 1, 3 do
  result = bit.bxor(i, 0xDEADC0DE)
end
