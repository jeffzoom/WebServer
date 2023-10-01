/*
 * @version: 
 * @Author: zsq 1363759476@qq.com
 * @Date: 2023-04-03 15:28:19
 * @LastEditors: zsq 1363759476@qq.com
 * @LastEditTime: 2023-05-16 14:02:28
 * @FilePath: /Linux_nc/WebServer_202304/myWebServer/threadpools.h
 * @Descripttion: 
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "lockerp.h"

using namespace std;

// 线程池类，将它定义为模板类是为了代码复用，模板参数T是任务类
template <typename T>
class threadpool {
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T* request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void* worker(void* arg); // 这个回调函数得是静态的，必须是静态的
    void run();

private:
    // 线程的数量
    int m_thread_number;

    // 请求队列中最多允许的、等待处理的请求的数量  
    int m_max_requests;
    
    // 描述线程池的数组，大小为m_thread_number    
    pthread_t * m_threads;

    // 是否结束线程          
    bool m_stop;
    
    // 请求队列
    std::list<T*> m_workqueue;  

    // 保护请求队列的互斥锁
    locker m_queuelocker;   // 互斥锁类 

    // 是否有任务需要处理
    sem m_queuestat;        // 信号量类

};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : 
    m_thread_number(thread_number), m_max_requests(max_requests), 
    m_stop(false), m_threads(nullptr) {

    if ((max_requests <= 0) || (max_requests <= 0)) {  // 系统鲁棒性更强
        throw std::exception();
    }

    m_threads = new pthread_t[thread_number];
    if (!m_threads) {   // 系统鲁棒性更强
        throw std::exception();
    }

    // 创建thread_number 个线程，并将他们设置为脱离线程。
    // 当子线程用完之后自己释放资源
    for (int i = 0; i < thread_number; i++) {

        printf( "create the %dth thread\n", i);

        // 开始了，这个worker就是回调函数，在这个函数里面处理数据
        // 错误：下面if中的worker写成了worker()
        if(pthread_create(m_threads + i, NULL, worker, this) != 0) { //创建thread_number个线程
            delete[] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])) {      //设置为脱离线程
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    delete [] m_threads;    // 释放资源
    m_stop = true;
}

template <typename T>
void* threadpool<T>::worker(void* arg) {    // 这个回调函数得是静态的，必须是静态的
    threadpool * pool = (threadpool *)arg;
    pool->run();
    return pool; 
}

template <typename T>
void threadpool<T>::run() {         // 回调函数嵌套

    while (!m_stop) {
        // 这个信号量什么时候有值的
        m_queuestat.wait();         // 对信号量加锁，调用一次信号量的值-1，如果值为0，就阻塞，
                                    // 判断有没有任务需要做
        m_queuelocker.lock();       // 加锁，多线程就要考虑数据安全，线程同步
        if (m_workqueue.empty()) {  // 请求队列为空则为1
            m_queuelocker.unlock(); // 解锁
            continue;;
        }

        // 请求队列操作
        T* request = m_workqueue.front();   //返回请求队列首元素的引用
        m_workqueue.pop_front();    // 删除第一个
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        
        request->process();         // 做任务，任务的函数先定义为process
                                    // 开始了，处理数据
    }
}

template <typename T>
bool threadpool<T>::append(T* request) { // 往工作队列里面添加任务

    // 操作工作队列时一定要加锁，因为它被所有线程共享。
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();     // 增加信号量
    return true;
}

#endif


