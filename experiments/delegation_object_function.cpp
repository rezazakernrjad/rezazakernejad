#include <iostream>
#include <functional>
using namespace std;
// C2 fetches c from class C1, passes it to class C3 and gets results.
class C1
{
    public:
    bool func(int& b)
    {
        cout << "Enter c in class c1: ";
        cin >> b;
        cout << "Class C1 c: " << b << endl;
        return true;
    }
};

class C2
{
    public:
    int a;
    bool func(int &b)
    {
        b++;
        cout << "Class C2 c: " << b << endl;
        return true;
    }
};

class C3
{
    public:
    int a;
    bool func(int &b)
    {
        int c = a * b;
        cout << "Class C3 c: " << c << endl;
        return true;
    }
};
//using C1_type = int (C1::*)(int&);
typedef bool(C1::* C1_type)(int&);
C1_type c1type = &C1::func;
C1 *c1;
typedef bool(C2::* C2_type)(int& b);
C2_type c2type = &C2::func;
C2 *c2;
int main(){
    int d;
    (c1->*c1type)(d);
    cout << "inMain: " << d << endl;
    
    (c2->*c2type)(d);
    cout << "in Main " << d << endl;
};