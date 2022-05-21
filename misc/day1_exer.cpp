#include <iostream>

using namespace std;

float res{};
float* result = &res;

void doCalc(float& a, float& b, char& op ){
switch (op)
{
    case '+':
        res = a + b;
    break;
    case '-':
        res = a - b;
    break;
}

}

int main(){
    float a = 2;
    float b = 3;
    char op = '-';
doCalc(a, b, op);
cout << "result is: " << *result << endl;
}