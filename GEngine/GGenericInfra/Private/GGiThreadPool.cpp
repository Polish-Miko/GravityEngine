#include "stdafx.h"
#include "GGiThreadPool.h"









void GGiThreadPool::Join()
{
	for (std::thread &worker : workers)
		worker.join();
}

void GGiThreadPool::Flush()
{
	// Wait for pipeline to be empty (i.e. all work is finished)
	while (!tasks.empty() || numRunningTasks != 0)
		std::this_thread::yield();
}

size_t GGiThreadPool::GetThreadNum()
{
	return numThread;
}


