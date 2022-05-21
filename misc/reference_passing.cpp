#include<iostream>
using namespace std;

void foo(int& c, int x){
    c++;
    x++;
}

int main(){
    int x = 1;
    int a = 1;
    foo(a, x);
    cout << "a in main after foo: "<<a << endl;
    cout << "x in main after foo: "<<x << endl;
}