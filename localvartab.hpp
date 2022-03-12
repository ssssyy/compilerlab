//用于记录每个函数内部变量分配的一个数据结构，一个函数一个，用于koopa2riscv
#include<iostream>
#include<cstdio>
#include<string>
#include<unordered_map>
using namespace std;

class LocalVarTable{
public:
    string func_name;   //指示这是哪个函数的表
    int cntsp;  //指示当前变量存放到哪里了，存放一个新变量就递增
    int totsize;    //指示这个栈帧的变量总长度（mod16取整后）
    unordered_map<string,int> table;  //用来存放这个函数内的所有符号的地址以及它们在栈上相对于sp的偏移
    
    LocalVarTable(string name_, int size_):func_name(name_),cntsp(0),totsize(size_){}
    virtual ~LocalVarTable(){}
    //查找，不在里面的报错并退出
    int Query(string name)
    {
        auto it = table.find(name);
        if(it==table.end())
        {
            cerr<<"no such element in local var table\n";
            exit(1);
        }
        return it->second;
    }
    void Insert(string name,int loc)
    {
        table.insert(make_pair(name,loc));
    }
    void Print()
    {
        cout<<"Print local sp location:"<<endl;
        for(auto it = table.begin(); it!=table.end(); it++)
        {
            cout<<it->first<<" "<<it->second<<endl;
        }
    }
};