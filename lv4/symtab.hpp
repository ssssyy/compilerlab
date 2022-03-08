#include<iostream>
#include<map>
#include<set>
#include<unordered_set>
#include<unordered_set>
#include<string>
#include<vector>

using namespace std;
class BaseAST;

class SymtabEntry{
public:
    string sym_name;    //符号名
    int ty;         //ty==0为常量，ty==1为变量，ty==2为函数
    SymtabEntry():sym_name(""),ty(-1){}
    SymtabEntry(int type_):sym_name(""),ty(type_){}
    SymtabEntry(string name_, int type_):sym_name(name_),ty(type_){}
    virtual ~SymtabEntry(){}
};

class ConstEntry: public SymtabEntry{
public:
    int const_val;  //常量的值
    ConstEntry(string name_,int val_):SymtabEntry(name_,0),const_val(val_){}
};

class VarEntry: public SymtabEntry{
public:
    string var_sym;     //变量的符号 @A
    VarEntry(string name_, string sym_):SymtabEntry(name_,1),var_sym(sym_){}
};


class Symtab{
public:
    vector<Symtab*> children;   //这个符号表的子符号表
    Symtab* parent;     //这个符号表的父符号表
    vector<SymtabEntry*> table;     //符号表的每一个条目
    int table_size;     //这个符号表里的条目数量
    //想不出构造函数咋写了，先不写了吧
    Symtab():parent(nullptr),table_size(0){}
    SymtabEntry* Query(string symname)
    {
        for(int i = 0; i < table_size; i++)
        {
            if(table[i]->sym_name==symname)
            {
                return table[i];
            }
        }
        return nullptr;
    }
    bool InsertConst(string symname, int val)
    {
        if(Query(symname)==nullptr)
        {
            auto tp = new ConstEntry(symname,val);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            return false;
        }
    }
    bool InsertVar(string varname, string varsym)
    {
        if(Query(varname)==nullptr)
        {
            auto tp = new VarEntry(varname,varsym);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            return false;
        }
    }
    void Print()
    {
        for(int i = 0; i < table_size; i++)
        {
            auto entry = table[i];           
            if(entry->ty==0)
            {
                cout<<"const "<<entry->sym_name<<" = ";
                ConstEntry* tp = (ConstEntry*)entry;
                cout<<tp->const_val<<endl;
            }
            else if(entry->ty==1)
            {
                cout<<"var "<<entry->sym_name<<" = ";
                VarEntry* tp = (VarEntry*)entry;
                cout<<tp->var_sym<<endl;
            }
        }
    }
};