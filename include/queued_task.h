#ifndef QUEUED_TASK_H_
#define QUEUED_TASK_H_

#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>

template <class T>
class queued_task
{
public:
    inline queued_task(size_t maxNumBuffer):maxNumBuffer{maxNumBuffer}{}

    using task = std::function<void(T data)>;

    inline void SetTask(task t)
    {
        task_do = t;
    }

    inline void StartTaskLoop()
    {
        if(running)
        {
            running = false;
            task_thread.join();
        }
        else
        {
            task_thread = std::thread([&]{
                running = true;
                while(running)
                {
                    std::unique_lock<std::mutex> lk(queue_mutex);

                    while(queue_buffer.empty())
                    {
                        if(!running) return;
                        queue_CV.wait(lk);
                    }

                    T data = std::move(queue_buffer.front());
                    queue_buffer.pop();
                    task_do(std::move(data));
                }
            });
        }
    }


    inline void EndTaskLoop()
    {
        if(running)
        {
            running = false;

            queue_CV.notify_all();
            task_thread.join();
            std::queue<T>().swap(queue_buffer); // clear queue            
        }
    }

    inline bool EnqueueData(T data)
    {
        if(queue_buffer.size() < maxNumBuffer && running)
        {
            std::unique_lock<std::mutex> lk(queue_mutex);
            queue_buffer.push(data);
            lk.unlock();
            queue_CV.notify_one();
            return true;
        }
        return false;
    }

private:
    std::thread task_thread;
    task task_do;

    bool running{false};
    size_t maxNumBuffer{3};
    std::queue<T> queue_buffer;

    std::mutex queue_mutex;
    std::condition_variable queue_CV;
};

#endif