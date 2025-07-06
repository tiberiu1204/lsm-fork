function fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

function printFibs(N) {
  for (let i = 0; i < N; i++) {
		fib(N)
    print("Hello World");
  }
}

let N = 10;
for(let i = 0; i < 100; i++) {
	printFibs(N);
}

