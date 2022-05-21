#include <thread>
#include <iostream>
#include <future>
using namespace std;


class Test
{
public:
  void runMultiThread();
private:
  int calculate(int from, int to);
  int calc(float x, float y);
};  

int Test::calc(float x, float y){
    cout << "from: " << x << endl;
    cout << "to: " << y << endl;
return x;
}

int Test::calculate(int from, int to){
cout << "from: " << from *to << endl;
cout << "to: " << to +from << endl;
return from *to;
}

// void Test::runMultiThread()
// {
//     std::thread t1(&Test::calculate, this,  50000, 10);
//     std::thread t2(&Test::calc, this, 11.0, 20.0);
//     t2.join();
//     t1.join();
// }

void Test::runMultiThread()
{
     auto f1 = std::async(&Test::calculate, this,  50000, 10);
     auto f2 = std::async(&Test::calc, this, 11.0, 20.0);

     auto res1 = f1.get();
     auto res2 = f2.get();
     cout << "res1: " << res1 << endl;
    cout << "res2: " << res2 << endl;
}

int main(){
    Test test;
    test.runMultiThread();
    return 0;
}