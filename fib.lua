import time

def fib(n):
    if n < 2:
        return n
    return fib(n-1) + fib(n-2)

# Benchmark
start = time.time()
for i in range(10):
    fib(30)
elapsed = time.time() - start

print(f"Python: fib(30) x10 = {elapsed*1000:.3f} ms")
