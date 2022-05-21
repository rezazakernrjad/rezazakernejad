#include <iostream>
#include <future>
#include <thread>

void f() {
        std::packaged_task<int()> task([]{ return 7; }); // wrap the function
        std::future<int> f1 = task.get_future();  // get a future
        std::thread t(std::move(task)); // launch on a thread
}

void f1() {
    std::future<int> f2 = std::async(std::launch::async, []{ return 8; });
}
//fire & forget
void f2() {
    std::promise<int> p;
    std::future<int> f3 = p.get_future();
    std::thread( [&p]{ p.set_value_at_thread_exit(9); }).detach();
}
    

int main() {
    
    // future from a packaged_task
    std::packaged_task<int()> task([]{return 1;}); // wrap the function
    std::future<int> f1 = task.get_future();  // get a future

    //std::thread t(std::move(task)); // launch on a thread
    std::thread t([]{
//        while(true);
    }); // launch on a thread
    t.join();
    std::cout << "is there anybody out there? " << std::endl;
 
    // future from an async()
    std::future<int> f2 = std::async(std::launch::async, []{ return 8; });
 
    // future from a promise
    std::promise<int> p;    
    std::future<int> f3 = p.get_future();   
     
    std::thread([&p]{
        p.set_value_at_thread_exit(9);
        size_t i = 0;
        while (true) {
            std::cout << "child thread " << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (i++ == 2) break;
            std::cout << "Im in here" << std::endl;
            /* code */
        }
//        p.set_value(2);
    }).detach();
    
    while(true) {
//        std::cout << "main thread " << std::endl;
        auto status = f3.wait_for(std::chrono::nanoseconds(0));
        if (status == std::future_status::ready) {
            std::cout << "THREAD IS DEAD!" << f3.get() << std::endl;
            break;
        }
    }
    return 0;
    
    
        // future from a promise
    std::promise<void> p1;
    std::future<void> f4 = p1.get_future();
    std::thread( [&p1]{         
        p1.set_value_at_thread_exit();
        size_t i = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (i++ == 3) {
               // p1.set_value();
                
                break;
            } 
            /* code */
        }
        }).detach();
    
    while(true) {
        auto status = f4.wait_for(std::chrono::nanoseconds(0));
        if (status == std::future_status::ready) break;
    }
    return 0;

            
    std::cout << "Waiting..." << std::flush;
    /*f1.wait();
    f2.wait();
    f3.wait();*/

    std::cout << "Done!\nResults are: "
              << /**/f1.get() << /**/' ' << f2.get() << ' ' << f3.get() << '\n';
    t.join();
    f();
}
