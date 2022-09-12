//用于记录每个函数内部变量分配的一个数据结构，一个函数一个，用于koopa2riscv
#include<iostream>
#include<cstdio>
#include<string>
#include<vector>
#include<unordered_map>
using namespace std;

class LocalVarTable{
public:
    string func_name;   //指示这是哪个函数的表
    int cntsp;  //指示当前变量存放到哪里了，存放一个新变量就递增
    int totsize;    //指示这个栈帧的变量总长度（mod16取整后）
    int para_num;   //指示有几个形参
    bool is_void;
    unordered_map<string,int> table;  //用来存放这个函数内的所有符号的地址以及它们在栈上相对于sp的偏移
    unordered_map<string,vector<int>> ptr_table;    //用来存放函数内部的指针的维数

    LocalVarTable(string name_, int begin_, int size_, int num_, bool void_):func_name(name_),cntsp(begin_),totsize(size_),para_num(num_),is_void(void_){}
    virtual ~LocalVarTable(){}
    //查找，不在里面的报错并退出
    int Query(string name)
    {
        auto it = table.find(name);
        if(it==table.end())
        {
            cerr<<name<<":no such element in local var table\n";
            return -1;
        }
        return it->second;
    }
    void Insert(string name,int loc)
    {
        table.insert(make_pair(name,loc));
    }
    void InsertPtr(string name,vector<int> size_)
    {
        ptr_table.insert(make_pair(name,size_));
    }
    bool IsArr(string name)
    {
        auto it = ptr_table.find(name);
        if(it!=ptr_table.end()) return true;
        return false;
    }
    int GetNextPtrSize(string name, vector<int>& tp)
    {
        auto it = ptr_table.find(name);
        int maxn = 1;
        if(name[0]!='0')
        {
            int len = it->second.size();
            for(int i = 1; i < len; i++)
            {
                tp.push_back(it->second[i]);
                maxn*=it->second[i];
            }
        }
        else
        {
            int len = it->second.size();
            for(int i = 0; i < len; i++)
            {
                tp.push_back(it->second[i]);
                maxn*=it->second[i];
            }
        } 
        return maxn;
    }
    void PrintPtr()
    {
        cout<<"Print ptr table:"<<endl;
        for(auto it = ptr_table.begin();it!=ptr_table.end();it++)
        {
            cout<<it->first<<" "<<it->second.size()<<endl;
            auto tp = it->second;
            for(int i = 0; i < tp.size(); i++)
            {
                cout<<tp[i]<<" ";
            }
            cout<<endl;
        }
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

class GlobalVarTableEntry{
public:
    string var_name;
    GlobalVarTableEntry(string name_):var_name(name_){}
};

class GlobalTable{
public:
    unordered_map<string,LocalVarTable*> ftable;   //存放所有函数的map
    unordered_map<string,GlobalVarTableEntry*> vtable;  //存放所有全局变量和常量
    unordered_map<string,vector<int>> ptr_table;    //存放全局数组的指针
    
    GlobalVarTableEntry* Query_globaltable(string name)
    {
        auto it = vtable.find(name);
        if(it==vtable.end())
        {
            cerr<<"No global var "<<name<<" table\n";
            return nullptr;
        }
        return it->second;
    }
    void Insert_globalvar(string name)
    {
        GlobalVarTableEntry* gv = new GlobalVarTableEntry(name);
        vtable.insert(make_pair(name,gv)); 
    }
    void Insert_globalptr(string name,vector<int> size_)
    {
        ptr_table.insert(make_pair(name,size_));
    }
    int GetNextPtrSize(string name, vector<int>& tp)
    {
        auto it = ptr_table.find(name);
        int maxn = 1;
        int len = it->second.size();
        for(int i = 1; i < len; i++)
        {
            tp.push_back(it->second[i]);
            maxn*=it->second[i];
        }
        return maxn;
    }
    bool IsArr(string name)
    {
        auto it = ptr_table.find(name);
        if(it!=ptr_table.end()) return true;
        return false;
    }
    LocalVarTable* Query_functable(string name)
    {
        auto it = ftable.find(name);
        if(it==ftable.end())
        {
            cerr<<"No func "<<name<<" table\n";
            return nullptr;
        }
        return it->second;
    }
    void Insert_functable(string func_name, LocalVarTable* ptr)
    {
        ftable.insert(make_pair(func_name,ptr));
    }
    void Print()
    {
        cout<<"Print Global Table:"<<endl;
        for(auto it = ftable.begin(); it!=ftable.end(); it++)
        {
            cout<<"func:"<<it->first<<endl;
            it->second->Print();
        }
        for(auto it = vtable.begin(); it!=vtable.end(); it++)
        {
            cout<<"global var:"<<it->first<<endl;
        }
    }
    void PrintPtr()
    {
        cout<<"Print ptr table:"<<endl;
        for(auto it = ptr_table.begin();it!=ptr_table.end();it++)
        {
            cout<<it->first<<" "<<it->second.size()<<endl;
            auto tp = it->second;
            for(int i = 0; i < tp.size(); i++)
            {
                cout<<tp[i]<<" ";
            }
            cout<<endl;
        }
    }
    void DeleteAll()
    {
        for(auto it = ftable.begin(); it!=ftable.end();it++)
        {
            delete(it->second);
        }
        for(auto it = vtable.begin(); it!=vtable.end();it++)
        {
            delete(it->second);
        }
    }
};