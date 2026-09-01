#ifndef __PLATFORMIO_H__
#define __PLATFORMIO_H__


class PlatformSocket;

class PlatformIO
{
public:
    static PlatformIO* GetInstance();
//private:
    PlatformIO();
    ~PlatformIO();
public:
    bool initIO();
    bool BindSocket( void* handle , void* overlapped );
    void UnBindSocket( void* handle);
    void runIO();
private:
    void RunSelect();
    void RunIocp();

private:
    int  m_EpollFd;  /**< epoll file descriptor */
};
#endif //__PLATFORMIO_H__
