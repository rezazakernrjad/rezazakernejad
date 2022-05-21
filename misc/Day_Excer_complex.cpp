#include <iostream>
using namespace std;

struct complexNr
{
    int real;
    int imag;
}comp1, comp2;
int main(){
    comp1.real = 2;
    comp1.imag = 3;
    comp2.real = 4;
    comp2.imag = 5;
    int real_nr = comp1.real + comp2.real;
    int imag_nr = comp1.imag + comp2.imag;
    cout << "real part: " << real_nr << "\nimage part: " << imag_nr << endl;
return 0;
}