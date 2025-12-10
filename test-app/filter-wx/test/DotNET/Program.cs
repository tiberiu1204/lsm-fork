using System;
using System.Runtime.CompilerServices;

class Program
{
    static readonly int pattern = unchecked((int)0xDEADBEEF);
    static int resultHolder; // No volatile needed
    static Random rng = new Random(); // Random number generator

    [MethodImpl(MethodImplOptions.NoInlining)]
    static int a(int n)
    {
        int result = 0;

        // Make loop large enough to prevent folding
        for (int i = 0; i < n; i++)
        {
            result += (i * 17) ^ (result >> 1);
        }

        // Store XOR to a field so it's observable
        resultHolder = result ^ pattern;
        return resultHolder;
    }

    static void Main()
    {
        // Warm up loop
        int sum = 0;
        for (int i = 0; i < 50_000; i++) 
        {
            int randomValue = rng.Next(i + 1); // random number between 0 and i (inclusive)
            sum += a(randomValue);
            sum %= pattern;
        }

        // Use the result so the JIT cannot optimize it away
        Console.WriteLine($"Checksum: {sum:X}");
    }
}


