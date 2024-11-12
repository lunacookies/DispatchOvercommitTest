#include <assert.h>
#include <dispatch/dispatch.h>
#include <errno.h>
#include <mach/mach.h>
#include <spawn.h>
#include <stdio.h>
#include <time.h>

enum { sentinelExitStatus = 92 };

static void sleepForMilliseconds(ptrdiff_t milliseconds) {
	struct timespec timespec = {0};
	timespec.tv_sec = milliseconds / 1000;
	timespec.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	nanosleep(&timespec, NULL);
}

static void performCPUBlockedWork(void) {
	uint32_t n = 0;
	for (ptrdiff_t i = 0; i < 10000000; i++) {
		n += arc4random_uniform(n);
	}
}

static void performKernelBlockedWork(void) {
	for (ptrdiff_t i = 0; i < 10000000; i++) {
		sleepForMilliseconds(100);
	}
}

typedef int Benchmark;
typedef enum {
	BenchmarkFlag_UseConcurrentQueue,
	BenchmarkFlag_TargetOvercommitQueue,
	BenchmarkFlag_PerformKernelBlockedWork,
	BenchmarkFlag__Count,
} BenchmarkFlag;

static char *benchmarkFlagNames[] = {
        "Use Concurrent Queue",
        "Target Overcommit Queue",
        "Perform Kernel-Blocked Work",
};

static Benchmark benchmarkFromFlag(BenchmarkFlag flag) {
	return 1 << flag;
}

static char *stringFromBenchmark(Benchmark benchmark) {
	char *string = calloc(BenchmarkFlag__Count + 1, 1);
	for (BenchmarkFlag flag = 0; flag < BenchmarkFlag__Count; flag++) {
		if (benchmark & benchmarkFromFlag(flag)) {
			string[flag] = '1';
		} else {
			string[flag] = '0';
		}
	}
	return string;
}

static Benchmark benchmarkFromString(char *string) {
	assert(strlen(string) == BenchmarkFlag__Count);
	Benchmark benchmark = 0;
	for (BenchmarkFlag flag = 0; flag < BenchmarkFlag__Count; flag++) {
		switch (string[flag]) {
			case '0': break;
			case '1': benchmark |= benchmarkFromFlag(flag); break;
			default: assert(false);
		}
	}
	return benchmark;
}

static dispatch_queue_t createQueueForBenchmark(Benchmark benchmark) {
	dispatch_queue_attr_t attr = DISPATCH_QUEUE_SERIAL;
	if (benchmark & benchmarkFromFlag(BenchmarkFlag_UseConcurrentQueue)) {
		attr = DISPATCH_QUEUE_CONCURRENT;
	}

	qos_class_t qos = QOS_CLASS_USER_INTERACTIVE;
	attr = dispatch_queue_attr_make_with_qos_class(attr, qos, 0);
	attr = dispatch_queue_attr_make_initially_inactive(attr);

	char *queueLabel = "org.xoria.DispatchOvercommitTest.BenchmarkQueue";
	dispatch_queue_t queue = dispatch_queue_create(queueLabel, attr);
	assert(queue);

	if (benchmark & benchmarkFromFlag(BenchmarkFlag_TargetOvercommitQueue)) {
		// By default all queues target one of the global overcommit queues.
	} else {
		// `dispatch_get_global_queue` returns us one of the global non-overcommit queues.
		dispatch_queue_t targetQueue = dispatch_get_global_queue(qos, 0);
		dispatch_set_target_queue(queue, targetQueue);
	}

	dispatch_activate(queue);

	return queue;
}

static void runBenchmark(Benchmark benchmark) {
	dispatch_queue_t queue = createQueueForBenchmark(benchmark);

	for (ptrdiff_t i = 0; i < 1000; i++) {
		if (benchmark & benchmarkFromFlag(BenchmarkFlag_PerformKernelBlockedWork)) {
			dispatch_async(queue, ^{
				performKernelBlockedWork();
			});
		} else {
			dispatch_async(queue, ^{
				performCPUBlockedWork();
			});
		}

		if (!(benchmark & benchmarkFromFlag(BenchmarkFlag_UseConcurrentQueue))) {
			dispatch_release(queue);
			queue = createQueueForBenchmark(benchmark);
		}
	}

	sleepForMilliseconds(5 * 1000);

	thread_act_array_t threads = NULL;
	mach_msg_type_number_t threadCount = 0;
	kern_return_t kernReturn = task_threads(mach_task_self(), &threads, &threadCount);
	assert(kernReturn == KERN_SUCCESS);
	printf("Ended with %d threads.\n", threadCount);

	exit(sentinelExitStatus);
}

static void entryPoint(void) {
	Benchmark lastBenchmark = benchmarkFromFlag(BenchmarkFlag__Count) - 1;
	for (Benchmark benchmark = 0; benchmark <= lastBenchmark; benchmark++) {
		printf("\nBenchmark Configuration:\n");
		for (BenchmarkFlag flag = 0; flag < BenchmarkFlag__Count; flag++) {
			printf("%30s: ", benchmarkFlagNames[flag]);
			if (benchmark & benchmarkFromFlag(flag)) {
				printf("Yes");
			} else {
				printf("No");
			}
			printf("\n");
		}

		char *binary = "DispatchOvercommitTest";
		char *arguments[] = {binary, stringFromBenchmark(benchmark), 0};

		pid_t runnerPID = 0;
		int returnCode = posix_spawn(&runnerPID, binary, 0, 0, arguments, 0);
		assert(returnCode == 0);
		assert(runnerPID > 0);

		// Wait for runner to complete before moving onto the next benchmark.
		while (true) {
			int status = 0;
			pid_t returnedPID = waitpid(runnerPID, &status, 0);
			if (returnedPID == -1 && errno == EINTR) {
				continue;
			}
			assert(returnedPID == runnerPID);
			assert(WEXITSTATUS(status) == sentinelExitStatus);
			break;
		}
	}

	exit(0);
}

int main(int argumentCount, char **arguments) {
	dispatch_async(dispatch_get_main_queue(), ^{
		switch (argumentCount) {
			case 1: entryPoint(); break;
			case 2: runBenchmark(benchmarkFromString(arguments[1])); break;
			default: assert(false);
		}
	});
	dispatch_main();
}
