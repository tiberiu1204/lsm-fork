local jit = require("jit")
local bit = require("bit") -- REQUIRED for integer math
local jdump = require("jit.dump")

-- 1. CLEAN SLATE: Flush any old traces so we start fresh
jit.flush()

-- 2. SETUP: Tell JIT to compile immediately (after 1 run)
jit.opt.start("hotloop=1", "hotexit=1")

-- 3. ENABLE LOGGING: Write the trace to a file so we can inspect it
jdump.on(nil, "trace.log")

-- 4. THE FUNCTION: Use bit.bxor to FORCE integer arithmetic
--    Standard '+' will default to floats (addsd). 
--    bit.bxor forces x86 integer instructions.
local function magic_function(val)
    return bit.bxor(val, 0xDEADBEEF)
end

print("--- [Ghost Run] Executing once to trigger compilation... ---")
-- The JIT sees this, hits the threshold (1), and compiles it.
magic_function(0)

print("--- [Real Run] This run is fully compiled machine code ---")
local result = magic_function(0)
print(string.format("Result: 0x%X", result))

-- 5. PROOF: Let's verify the trace contained our instruction
jdump.off()
print("\n--- Checking Trace for Magic Number ---")
local f = io.open("trace.log", "r")
local content = f:read("*all")
f:close()

-- Look for the XOR instruction in the log
if content:find("xor") or content:find("3735928559") or content:find("deadbeef") then
    print("SUCCESS: Found integer instruction with DEADBEEF!")
    -- Print the specific lines
    for line in content:gmatch("[^\r\n]+") do
        if line:find("xor") or line:find("deadbeef") then print("TRACE LINE: " .. line) end
    end
else
    print("WARNING: Still using floating point math. Did you use bit.bxor?")
    print(content)
end
