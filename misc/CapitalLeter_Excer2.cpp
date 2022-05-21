#include <iostream>

using namespace std;
int main(){
    char c, C;
    cout << "Enter character: ";
    cin >> c;
    if (int(c) < 122 && int(c) > 97){
        int(C)= int(c) - 32;
        printf("Capotal leter of %c is %c \n", int(c), int(C));
    } else if (int(c) < 90 && int(c) > 65){
        cout << c << ": is capital"<< endl;
    } else {
        cout << c << ": is not a character"<< endl;
    }

}