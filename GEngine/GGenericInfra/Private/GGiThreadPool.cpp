#include "stdafx.h"
#include "GGiThreadPool.h"







void GGiThreadPool::JoinAll()
{
	for (std::thread &worker : workers)
		worker.join();
}


