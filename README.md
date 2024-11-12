Results on a 10-core M1 Pro running macOS 15.1:

```
Benchmark Configuration:
          Use Concurrent Queue: No
       Target Overcommit Queue: No
   Perform Kernel-Blocked Work: No
Ended with 11 threads.

Benchmark Configuration:
          Use Concurrent Queue: Yes
       Target Overcommit Queue: No
   Perform Kernel-Blocked Work: No
Ended with 11 threads.

Benchmark Configuration:
          Use Concurrent Queue: No
       Target Overcommit Queue: Yes
   Perform Kernel-Blocked Work: No
Ended with 513 threads.

Benchmark Configuration:
          Use Concurrent Queue: Yes
       Target Overcommit Queue: Yes
   Perform Kernel-Blocked Work: No
Ended with 11 threads.

Benchmark Configuration:
          Use Concurrent Queue: No
       Target Overcommit Queue: No
   Perform Kernel-Blocked Work: Yes
Ended with 65 threads.

Benchmark Configuration:
          Use Concurrent Queue: Yes
       Target Overcommit Queue: No
   Perform Kernel-Blocked Work: Yes
Ended with 65 threads.

Benchmark Configuration:
          Use Concurrent Queue: No
       Target Overcommit Queue: Yes
   Perform Kernel-Blocked Work: Yes
Ended with 513 threads.

Benchmark Configuration:
          Use Concurrent Queue: Yes
       Target Overcommit Queue: Yes
   Perform Kernel-Blocked Work: Yes
Ended with 65 threads.
```
