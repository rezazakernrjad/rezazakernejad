#include <iostream>

using namespace std;
float fahr{0};
float* fahrPtr = &fahr;

void fahrFcn(float cels){
    cout << "got cels" << endl;
    fahr = (cels*9/5) +32;
}
void ascii(){
    char c;
    cout<< "enter chanarcter: ";
    cin >> c;
    printf("ASCII of user entered character is: %d \n", c);
}
int main(){
    float cels{0.0F};
    cout << "enter celcius: ";
    cin >> cels;
    fahrFcn(cels);
    cout << "fahrenheit: " << *fahrPtr << endl;
    ascii();
return 0;
}