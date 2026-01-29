// target.js

// 1. Force Baseline JIT to compile immediately
setJitCompilerOption("baseline.warmup.trigger", 0);

// 2. Force IonMonkey (optimizing JIT) to compile immediately
setJitCompilerOption("ion.warmup.trigger", 0);

function heavyCalculation(x) {
  // This math prevents the optimizer from dead-code eliminating the function entirely
  return Math.imul(x ^ 0xDEADBEEF, 0x12344449) >> 2;
}

// 3. Run once. With the settings above, this SINGLE call will trigger JIT compilation.
//    Your LSM should receive the mmap/mprotect event during this call.
var result = heavyCalculation(42);

print("Calculation done: " + result);
