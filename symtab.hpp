#include<iostream>
#include<map>
#include<set>
#include<unordered_set>
#include<unordered_set>
#include<string>
#include<vector>

using namespace std;
class BaseAST;
extern vector<int> symtabnum;
class SymtabEntry{
public:
    string sym_name;    //符号名
    int ty;         //ty==0为常量，ty==1为变量，ty==2为函数
    int val;        //暂时只在用于为常量初始化时使用
    SymtabEntry():sym_name(""),ty(-1),val(0){}
    SymtabEntry(int type_):sym_name(""),ty(type_),val(0){}
    SymtabEntry(string name_, int type_):sym_name(name_),ty(type_),val(0){}
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
    Symtab* parent;     //这个符号表的父符号表,根符号表的parent设置为nullptr
    vector<SymtabEntry*> table;     //符号表的每一个条目
    int table_size;     //这个符号表里的条目数量
    int layer;
    int child_no;   //表示这是第几个子表
    //想不出构造函数咋写了，先不写了吧
    Symtab():parent(nullptr),table_size(0),layer(0),child_no(0){}
    Symtab(int layer_):parent(nullptr),table_size(0),layer(layer_){}
    SymtabEntry* Query(string symname)
    {
        //Print();
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
        cout<<"Layer "<<layer<<endl;
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
    //支持递归向上查询符号的函数，其他功能同Query
    SymtabEntry* QueryUp(string symname)
    {
        //cout<<"Query:"<<symname<<endl;
        Symtab* cnttab = this;
        do
        {
            auto tp = cnttab->Query(symname);
            if(tp!=nullptr)
            {
                return tp;
            }
            cnttab = cnttab->parent;
        } 
        while (cnttab->parent!=nullptr);
        return nullptr;    
    }
    //在cnt的基础上新建一个symtab，并且返回这个新的symtab
    Symtab* NewSymTab()
    {
        Symtab* newone = new Symtab(this->layer+1);
        if(symtabnum.size()==layer+1) symtabnum.push_back(0);
        newone->child_no = symtabnum[layer+1];
        symtabnum[layer+1]++;
        newone->parent = this;
        return newone;
    }
    //删除cnt符号表，返回它的上一层符号表
    void DeleteSymTab()
    {
        //Symtab* up = this->parent;
        delete this;
    }
};