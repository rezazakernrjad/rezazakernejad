#include <iostream>
#include <condition_variable>
#include <thread>
#include <chrono>
 
std::condition_variable cv;

std::mutex cv_m; // This mutex is used for three purposes:
                 // 1) to synchronize accesses to i
                 // 2) to synchronize accesses to std::cerr
                 // 3) for the condition variable cv
int i = 0;

void waits()
{
   while (true) {
    std::unique_lock<std::mutex> lk(cv_m);    
    std::cerr << "Waiting... in thread " << std::this_thread::get_id() << std::endl;
    cv.wait(lk, []{return i == 1;});
    std::cerr << "...finished waiting. i == 1\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
//    i = 0;

    }
}
 
void signalME() {
   
    std::this_thread::sleep_for(std::chrono::seconds(2));
    bool bol = true;
  while (bol)
  {
      /* code */
      bol = false;
 
    {
        std::lock_guard<std::mutex> lk(cv_m);
        i = 1;//std::rand()%2;
        std::cerr << "Notifying ...\n";
    }
      cv.notify_all();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lk(cv_m);
        i = 0;//std::rand()%2;
//        std::cerr << "Notifying ...\n";
//        cv.notify_all();
    }
    break;
  }



            std::cerr << "signaling finished \n";
}
 
int main()
{
    std::thread t1(waits), t2(waits), t3(waits), t4(signalME);
    t1.join(); 
    t2.join(); 
    t3.join();
    t4.join();
}
