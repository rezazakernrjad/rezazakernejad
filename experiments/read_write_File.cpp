#include <iostream>
#include <fstream>
#include <string>
using namespace std;
int main(){
   fstream newfile;
/*   newfile.open("tpoint.txt",ios::out);  // open a file to perform write operation using file object
   if(newfile.is_open()) //checking whether the file is open {
      newfile<<"Tutorials point \n"; //inserting text
      newfile.close(); //close the file object
*/   
   newfile.open("tpoint.txt",ios::in); //open a file to perform read operation using file object
   if (newfile.is_open()){ //checking whether the file is open
      string tp, title, author, publisher;
      string book_attr{""};
      int price_int{};
      int copies_int{};
      while(getline(newfile, tp)){ //read data from file object and put it into string.
      book_attr = tp.substr(0, 7);
//         cout << tp << "\n"; //print the data of the string
         if (tp.substr(0, 7) == "title :"){
            title = tp.substr(8,40);
            cout << "chatched title: " << title << endl;
         } else if(tp.substr(0, 7) == "author:") {
            author = tp.substr(8,40);
            cout << "chatched author: " << author << endl;
         } else if (tp.substr(0, 7) == "publish:"){
            publisher = tp.substr(8,40);
            cout << "chatched publisher: " << publisher << endl;
         } else if (tp.substr(0, 7) == "price :"){
            price_int = stoi(tp.substr(8,8));
            cout << "chatched int Price + 1 : " << price_int +1 << endl;
         } else if (tp.substr(0, 7) == "copies :"){
            copies_int = stoi(tp.substr(8,8));
            cout << "chatched int cpies + 1 : " << copies_int +1 << endl;
         } else {
            cout << "end of book" << endl;
         }
      }
      newfile.close(); //close the file object.
   }
   return 0;
}
