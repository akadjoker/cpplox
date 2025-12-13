# empty_bench.py
import time

def empty():
    return None

iterations = 10
calls = 100000

times = []
for _ in range(iterations):
    start = time.perf_counter()
    for i in range(calls):
        empty()
    elapsed = time.perf_counter() - start
    times.append(elapsed * 1000)

avg = sum(times) / len(times)
best = min(times)

print(f"Python Empty Calls:")
print(f"  {calls} calls")
print(f"  Avg:  {avg:.2f} ms ({calls/avg:.0f} calls/ms)")
print(f"  Best: {best:.2f} ms ({calls/best:.0f} calls/ms)")
print(f"  = {(calls/best)*1000:.0f} calls/second")
