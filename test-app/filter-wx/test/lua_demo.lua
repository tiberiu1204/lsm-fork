local jit = require("jit")
local bit = require("bit")

jit.opt.start("hotloop=1")

function marker(x)
  -- constants that will appear in machine code
  local a = 0xDEADBEEF
  local b = 0xCAFEBABE
  local c = 0xFEEDFACE

  x = bit.bxor(x, a)
  x = x + b
  x = bit.band(x, c)
  return x
end

-- trigger JIT
for i = 1, 200 do
  marker(i)
end

-- dummy print to keep LuaJIT from optimizing away
print(marker(123))
