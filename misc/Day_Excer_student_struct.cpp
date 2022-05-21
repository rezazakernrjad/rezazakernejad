#include <iostream>
#include<string>
using namespace std;

struct student
{
    string  s_name;
    int id;
    float grade;
    void printFcn();
}group_student[10];

int main(){
    student st_obj[2], *std_p;
    std_p = &st_obj[2];
//    group_student[2] = &st_obj;
//    for (int i = 0; i < 2; i++){
//        cout << "Enter name of student number " << i << ": ";
//        string p_name = "Reza";

        group_student->s_name = "Reza";
        group_student[1]->s_name = "Tiam";
        cout << "student name pointer 0: "<< group_student->s_name << endl;
        cout << "student name pointer 1: "<< group_student[1]->s_name << endl;
//        cout << "student name obj: "<< st_obj.s_name << endl;
//        group_student[1]->s_name = "Tiam";
//        getline(cin, group_student[i]->s_name);
//        group_student[i]->s_name = p_name;
//        cout << "student name: "<< group_student[i]->s_name<< endl;
//        cout << "Enter id of student number " << i<< ": ";
//        cin >> group_student[i]->id;
//        cout << "Enter grade of student number " << i<< ": ";
//        cin >> group_student[i]->grade;
        
//    }
//  group_student[0]->printFcn();
return 0;
}
/*void student::printFcn(){
    for (int k = 0; k < 2; k++){
        cout << "student " << k << " name: " << group_student[k]->s_name << endl; 
        cout << "student " << k << " id: " << group_student[k]->id << endl;
        cout << "student " << k << " grade: " << group_student[k]->grade << endl;
    }
}*/