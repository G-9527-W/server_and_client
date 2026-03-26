#include <iostream>
#include<arpa/inet.h>
#include <string>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <thread>
#include <queue>
#include <vector>
#include<mutex>
#include <condition_variable>
#include <chrono>
#include <future>
#include <functional>
#include <type_traits>
using namespace std;
template<typename T>
class thread_safe_queue
{
	condition_variable cv;
	mutable mutex mut;
	queue<T>q;
public:
	thread_safe_queue() = default;
	~thread_safe_queue() = default;
	thread_safe_queue(const thread_safe_queue&) = delete;
	thread_safe_queue& operator=(const thread_safe_queue&) = delete;
	void push(T value)
	{
		lock_guard<mutex>lock(mut);
		q.push(move(value));
		cv.notify_one();

	}
	void wait_pop(T& value)
	{
		unique_lock<mutex>lock(mut);
		cv.wait(lock, [this]() {return !this->q.empty(); });
		value = move(q.front());
		q.pop();
	}
	bool try_pop(T& value)
	{
		lock_guard<mutex>lock(mut);
		if (q.empty())
		{
			return false;
		}
		value = move(q.front());
		q.pop();
		return true;
	}
	bool wait_for_pop(T& value, chrono::milliseconds timeout)
	{
		unique_lock<mutex>lock(mut);
		if (!cv.wait_for(lock, timeout, [this] {return !this->q.empty(); }))
		{
			return false;
		}
		value = move(q.front());
		q.pop();
		return true;
	}
	bool is_empty()const
	{
		lock_guard<mutex>lock(mut);
		return q.empty();
	}
	size_t size() const
	{
		lock_guard<mutex>lock(mut);
		return q.size();
	}
	void clear()
	{
		lock_guard<mutex>lock(mut);
		while (!q.empty())
		{
			q.pop();
		}
	}
	void notify_all()
	{
		cv.notify_all();
	}
};


class function_pack
{
private:

	class impl_base
	{
	public:
		virtual void call() = 0;
		virtual ~impl_base() = default;
	};

	unique_ptr <impl_base>impl;
	template<typename F>
	class impl_type :public impl_base
	{
	public:
		F f;
		impl_type(F&& temp_f) :f(move(temp_f))
		{

		}
		void call()
		{
			f();
		}
	};
public:
	template<typename F>
	function_pack(F&& temp_f) :impl(new impl_type<F>(move(temp_f)))
	{
	}
	function_pack() = default;

	void operator()()
	{
		if (impl) impl->call();

	}
	function_pack(function_pack&& fp) noexcept :impl(move(fp.impl))
	{

	}
	function_pack& operator=(function_pack&& fp)noexcept
	{
		impl = move(fp.impl);
		return *this;
	}

	function_pack(const function_pack& fp) = delete;
	function_pack& operator=(const function_pack& fp) = delete;


};
class local_deque
{
private:
	mutable mutex mut;
	deque <function_pack>local_d;
public:
	local_deque() = default;
	local_deque(const local_deque& other) = delete;
	local_deque& operator=(const local_deque& othrer) = delete;

	void push_front(function_pack task)
	{
		lock_guard<mutex>lock(mut);
		local_d.push_front(move(task));
	}
	bool empty()const
	{
		lock_guard<mutex> lock(mut);
		return local_d.empty();
	}
	bool try_pop(function_pack &task)
	{
		
		lock_guard<mutex>lock(mut);
		if (local_d.empty())
		{
			return false;
		}
		task = move(local_d.front());
		local_d.pop_front();
		return true;
	}
	bool try_steal(function_pack& task)
	{
		
		lock_guard<mutex>lock(mut);
		if (local_d.empty())
		{
			return false;
		}
		task = move(local_d.back());
		local_d.pop_back();
		return true;
	}

};
class thread_pool
{
private:
    thread_safe_queue<function<void()>> work_q;
    vector<thread> threads;
    atomic<bool> done = false;
    unsigned thread_num = thread::hardware_concurrency() > 2 ? thread::hardware_concurrency() : 2;

    void worker_thread()
    {
        while (!done)
        {
            function<void()> task;
            if (work_q.try_pop(task))
            {
                task();
            }
            else
            {
                this_thread::yield();
            }
        }
    }

public:
    thread_pool()
    {
        try
        {
            for (unsigned i = 0; i < thread_num; ++i)
            {
                threads.emplace_back(&thread_pool::worker_thread, this);
            }
        }
        catch (...)
        {
            stop();
            throw;
        }
    }

    template<typename func>
    future<invoke_result_t<func>> submit(func f)
    {
        using type = invoke_result_t<func>;
        auto task = make_shared<packaged_task<type()>>(move(f));
        future<type> res = task->get_future();
        work_q.push([task]() { (*task)(); });
        return res;
    }

    void stop()
    {
        done = true;
        work_q.notify_all();
        for (auto& t : threads)
        {
            if (t.joinable()) t.join();
        }
    }

    ~thread_pool()
    {
        stop();
    }
};
void client_work(int c_fd,int c_port,string c_ip)
{
    std::cout<<"客户端:"<<c_ip<<"端口:"<<c_port<<"已接入"<<endl;
    char buff[1024];
   
    while(true)
    {
        memset(buff,0,sizeof(buff));
        int len=recv(c_fd,buff,sizeof(buff),0);
        if(len==0)
        {
            cout<<"客户端:"<<c_ip<<"已断开"<<endl;
            close(c_fd);
            break;
        }
        if(len<0)
        {
            cerr<<"客户端:"<<c_ip<<" 接入失败"<<endl;
            close(c_fd);
            return ;
        }
        string client_s(buff,len);
        cout<<"客户端:"<<c_ip<<"传入"<<client_s<<endl;
        int ret_s=send(c_fd,buff,len,0);
        if(ret_s<0)
        {
            cerr<<"客户端:"<<c_ip<<" 传回失败"<<endl;
            close(c_fd);
            return ;
        }
     
    }
}
int main()
{
    thread_pool pool;
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        cerr<<"socket is wrong";
        close(fd);
        return -1;
    }
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(8898);
    saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    int ret=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(ret<0)
    {
        cerr<<"bind is wrong";
        close(fd);
        return -1;
    }
    ret=listen(fd,128);
    if(ret<0)
    {
        cerr<<"listen is wrong";
        return -1;
    }
  

    while(true)
    {
        struct sockaddr_in caddr;
        socklen_t size_len=sizeof(caddr);
        int cfd=accept(fd,(struct sockaddr*)&caddr,&size_len);
        if(cfd<0)
       {
           cerr<<"accept is wrong";
           return -1;

        }
        char ip[32];
        inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip,sizeof(ip));
        string client_ip(ip);
        int cport=ntohs(caddr.sin_port);
        pool.submit([=]{client_work(cfd,cport,client_ip);});
        
     

    }
    close(fd);
}
