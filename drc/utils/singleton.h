/**
 * @file singleton.h
 * @brief 单例模式封装
 * @author ershisi
 * @date 2021-12-28
 */
#ifndef __DRC_SINGLETON_H__
#define __DRC_SINGLETON_H__

#include <memory>
/**
 * @brief 单例模式封装类
 * @details T 类型
 */
template<class T>
class Singleton 
{
public:
    /**
     * @brief 返回单例裸指针
     */
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 */
template<class T>
class SingletonPtr 
{
public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};



#endif /*__DRC_SINGLETON_H__*/
