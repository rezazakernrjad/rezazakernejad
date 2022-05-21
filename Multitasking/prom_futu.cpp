
#include <iostream>
#include <thread>
#include <future>
#include<chrono>
void initiazer(std::promise<int> * promObj)
{
    std::cout<<"Inside Thread"<<std::endl; 
//    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i< 100; i++){
        std::cout<<"INSIDE THREAD*****"<<std::endl; 
//        if (i == 50) 
        std::cout<<" IIIIIIIII: " << i++<<std::endl;
        promObj->set_value(i);
    }
}
void calc_p(std::future<int>* val){
    std::cout<<"Thread calc p"<<std::endl;
//    std::cout << "Val in thread calc: " << val->get() << std::endl;
}
int main()
{
    std::promise<int> promiseObj;
    std::future<int> futureObj = promiseObj.get_future();
    std::thread th_init(initiazer, &promiseObj);
    int x=0;
    for (int i = 0; i< 100; i++){
        x = futureObj.get();
        std::cout<<"DO something while wating for result"<<std::endl; 
        std::cout<<" XXXXXXXXX: " << x<<std::endl;
    }
//    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout<<"**************************promised value: " << x<<std::endl;
    
    th_init.join();
    return 0;
}