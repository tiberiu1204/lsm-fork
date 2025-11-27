using System;

class Program
{
    // A method that becomes hot enough to be JIT-compiled
    static long HotMethod(long n)
    {
        long result = 0;
        for (long i = 0; i < n; i++)
        {
            result += (i * 17) ^ (result >> 1);
        }
        return result ^ 0xdeadbeef;
    }

    static void Main()
    {
        // Warm up so RyuJIT compiles HotMethod()
        for (int i = 0; i < 50_000; i++)
        {
            HotMethod(5000);
        }

        Console.WriteLine("DotNET demo finished\n");
    }
}

