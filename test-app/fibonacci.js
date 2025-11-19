function fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

function printFibs(N) {
  for (var i = 0; i < N; i++) {
    fib(N);
    // print("Hello World");
  }
}

var N = 10;
for (var i = 0; i < 10000; i++) {
  printFibs(N);
}

