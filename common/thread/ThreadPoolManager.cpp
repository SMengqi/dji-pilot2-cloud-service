#include "ThreadPoolManager.h"

ThreadPoolManager::ThreadPoolManager() :
    m_ioService(std::make_unique<boost::asio::io_service>()),
    m_work(std::make_unique<boost::asio::io_service::work>(*m_ioService)),
    m_threadPool(std::make_unique<boost::thread_group>())
{
}

ThreadPoolManager::~ThreadPoolManager()
{
    stop();
}

void ThreadPoolManager::start(size_t threadCount)
{
    for (size_t i = 0; i < threadCount; ++i) {
        m_threadPool->create_thread([this] { m_ioService->run(); });
    }
}

void ThreadPoolManager::stop()
{
    if (m_work) {
        m_work.reset(); // 停止工作对象，允许io_service完成
    }
    
    if (m_ioService) {
        m_ioService->stop();
    }
    
    if (m_threadPool) {
        m_threadPool->join_all();
        m_threadPool.reset();
    }
}

void ThreadPoolManager::postTask(std::function<void()> task)
{
    if (m_ioService) {
        m_ioService->post(std::move(task));
    }
}