
#include<cstdio>
#include<iostream>
#include<ctime>
#include<cstring>
using namespace std;


int main (){
struct S {
  unsigned short aa: 4;
  unsigned short bb: 2;
  unsigned short cc: 2;
  unsigned short dd: 3;
  unsigned short ee: 3;
  unsigned short ff: 2;
}s;
uint8_t B[2] = {0b11011001, 0b10010110};
memcpy(&s, &B, 2);
//s = {0b1011, 0b11, 0b1};
S *ptr;
ptr = &s;
printf("ptr %x \n", *ptr);
printf("aa %x \n", s.aa);
printf("bb %x \n", s.bb);
printf("cc %x \n", s.cc);
printf("dd %x \n", s.dd);
printf("ee %x \n", s.ee);
printf("ff %x \n", s.ff);
cout << "size " << sizeof(s) << endl;
}
// void Delay(clock_t delay){
//     clock_t now = clock();
//     while(clock() - now <delay/10);
//     cout<<"Message Show after delay that you entered "<< clock() - now << endl;int main();
// }
// int main(){
//     clock_t d[]{2 * CLOCKS_PER_SEC,
//                 10 * CLOCKS_PER_SEC,
//                 15 * CLOCKS_PER_SEC,
//                 25 * CLOCKS_PER_SEC,
//                 35 * CLOCKS_PER_SEC};
//     uint8_t gear{0};

//     for (int i = 0 ; i < 5; i++){
//         Delay(d[i]);
//         printf ("Gear %d \n ",gear++);}

// return 0;

//  }