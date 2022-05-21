#include <iostream>

using namespace std;
void BubbleSort(int* arr, int size ){
    int tmp{0};
    for (int j = 0; j < size  ; j++){
        for (int i = 0; i < size ; i++){
            if (arr[i] > arr[i+1]){
                tmp = arr[i];
                arr[i] = arr[i+1];
                arr[i+1] = tmp;
            }
        }
    }

}

int main(){
    int size{0};
    cout << "enter size of array: ";
    cin >> size;
    int arr[size];
    for (int i = 0 ; i < size ; i++){
        cout << "enter element " << i << " off array: ";
        cin >>  arr[i];
    }
    BubbleSort(arr, size);
    for (int k = 0; k < size; k++){
        cout << "ellement[k= "<<k<<"]" << arr[k] << endl;
    }
    return 0;
}