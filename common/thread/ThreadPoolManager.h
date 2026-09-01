#ifndef THREADPOOLMANAGER_H_
#define THREADPOOLMANAGER_H_

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <functional>
#include <memory>

class ThreadPoolManager {
public:
    ThreadPoolManager();
    ~ThreadPoolManager();
    
    void start(size_t threadCount = 4);
    void stop();
    void postTask(std::function<void()> task);
    
    // 禁止拷贝
    ThreadPoolManager(const ThreadPoolManager&) = delete;
    ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
    
private:
    std::unique_ptr<boost::asio::io_service> m_ioService;
    std::unique_ptr<boost::asio::io_service::work> m_work;
    std::unique_ptr<boost::thread_group> m_threadPool;
};

#endif /* THREADPOOLMANAGER_H_ */