
#ifndef __SCOPEDLOCK_H__
#define __SCOPEDLOCK_H__
template <class M>
class ScopedLock
    /// A class that simplifies thread synchronization
    /// with a mutex.
    /// The constructor accepts a Mutex (and optionally
    /// a timeout value in milliseconds) and locks it.
    /// The destructor unlocks the mutex.
{
public:
    explicit ScopedLock(M& mutex): _mutex(mutex)
    {
        _mutex.lock();
    }
    
    ~ScopedLock()
    {
        _mutex.unlock();//lint !e1551
    }
private:
    ScopedLock& operator = ( const ScopedLock& obj )
    {
        return *this;
    }

    M& _mutex;
};

#endif //__SCOPEDLOCK_H__
