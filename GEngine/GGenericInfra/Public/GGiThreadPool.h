#pragma once
#include "GGiPreInclude.h"





typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;

class GGiThreadPool
{

public:

	GGiThreadPool(size_t numThreads);

	~GGiThreadPool();

	template<class F> void Enqueue(F f);

	void Join();

private:

	asio_worker mWork;

	boost::asio::io_service mService;

	boost::thread_group mThreadGroup;

};

