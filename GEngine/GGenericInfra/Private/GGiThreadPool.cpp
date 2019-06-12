#include "stdafx.h"
#include "GGiThreadPool.h"






GGiThreadPool::GGiThreadPool(size_t threads) :mService(), mWork(new asio_worker::element_type(mService))
{
	for (std::size_t i = 0; i < threads; ++i)
	{
		auto worker = boost::bind(&boost::asio::io_service::run, &(this->mService));
		mThreadGroup.add_thread(new boost::thread(worker));
	}
}

GGiThreadPool::~GGiThreadPool()
{
	mWork.reset(); //allow run() to exit
	mThreadGroup.join_all();
	mService.stop();
}

template<class F>
void GGiThreadPool::Enqueue(F f)
{
	service.post(f);
}

void GGiThreadPool::Join()
{
	mThreadGroup.join_all();
}



