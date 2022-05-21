#include<iostream>
#include<functional>
using namespace std;

template <class T>
bool Foo(T foo){
    // return value of functions Bar1 and Bar2
    cout << "Just a number: " << foo << endl;
}

bool Bar1(int x){
    x++;
    cout << "in Bar1: "<< x << endl;
    return false;
}

bool Bar2(int x, int y){
    int z = x* y;
    cout << "in Bar2: "<< z << endl;
    return true;
}

//Function pointer
using function_ptr1 = bool(*)(int);
using function_ptr2 = bool(*)(int, int);
function_ptr1 fcnptr = Bar1;
function_ptr2 fcnptr2 = Bar2;

//std::function
std::function<bool(int)> std_function1 = std::bind(&Bar1, std::placeholders::_1);
std::function<bool(int, int)> std_function2 = 
                std::bind(&Bar2, std::placeholders::_1, std::placeholders::_2);


int main(){
    Foo(function_ptr1 (fcnptr)(5));
    Foo(function_ptr2 (fcnptr2)(3, 6));

    Foo(std_function1(9));
    Foo(std_function2(3,3));
    int number = 99;
    Foo(number);
    return 0;
}