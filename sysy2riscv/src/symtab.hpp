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
    int ty;         //ty==0为常量，ty==1为变量，ty==2为函数，ty==3为数组，ty==4为指针
    int val;        //暂时只在用于为常量初始化时使用
    bool is_param;  //函数的参数是可以存在同名变量于是被覆盖的
    SymtabEntry():sym_name(""),ty(-1),val(0),is_param(false){}
    SymtabEntry(int type_):sym_name(""),ty(type_),val(0),is_param(false){}
    SymtabEntry(string name_, int type_):sym_name(name_),ty(type_),val(0),is_param(false){}
    virtual ~SymtabEntry(){}
};

class ConstEntry: public SymtabEntry{
public:
    int const_val;  //常量的值
    ConstEntry(string name_,int val_):SymtabEntry(name_,0),const_val(val_){}
};

class VarEntry: public SymtabEntry{
public:
    string var_sym;     //变量的符号 @A_xx_xx
    VarEntry(string name_, string sym_):SymtabEntry(name_,1),var_sym(sym_){}
};

class FuncEntry: public SymtabEntry{
public:
    string func_name;   //函数的名字 @main，@f
    bool void_func;
    FuncEntry(string name_, bool void_):SymtabEntry(name_,2),func_name(name_),void_func(void_){}
};

class ArrEntry: public SymtabEntry{
public:
    string arr_sym;     //数组的符号 @arr_xx_xx
    bool is_const;      //标记是否是常量数组
    int arr_d;      //数组的维数，例如int a[3][2]，这一项为2
    vector<int> arr_size;   //数组每一维的长度，例如上述的数组这一项为3,2
    int arr_total_size;
    ArrEntry(string name_, string sym_, bool const_,int D_, vector<int> &size_):SymtabEntry(name_,3),arr_sym(sym_),is_const(const_),arr_d(D_)
    {
        int maxn = 1;
        for(int i = 0; i <D_; i++)
        {
            arr_size.push_back(size_[i]);
            maxn*=size_[i];
        }
        arr_total_size = maxn;
    }
};

class PtrEntry: public SymtabEntry{
public:
    string ptr_sym;
    int arr_d;
    vector<int> arr_size;
    string alloc_string;
    PtrEntry(string name_, string sym_, int D_,vector<int> &size_, string alloc_):SymtabEntry(name_,4),ptr_sym(sym_),arr_d(D_),alloc_string(alloc_)
    {
        arr_size.push_back(0);  //第一维直接置为0
        for(int i = 0; i < D_-1; i++)
        {
            arr_size.push_back(size_[i]);
        }
    }
};

class Symtab{
public:
    vector<Symtab*> children;   //这个符号表的子符号表
    Symtab* parent;     //这个符号表的父符号表,根符号表的parent设置为nullptr
    vector<SymtabEntry*> table;     //符号表的每一个条目
    int table_size;     //这个符号表里的条目数量
    int layer;
    int child_no;   //表示这是第几个子表
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
    bool GetNextPtr(string name,vector<int>& v)
    {
        auto tp = QueryUp(name);
        if(tp==nullptr)
        {
            cout<<"name = "<<name<<endl;
            return false;
        }
        if(tp->ty==3)
        {
            auto ttp = (ArrEntry*) tp;
            for(int i = 1; i < ttp->arr_d; i++)
            {
                v.push_back(ttp->arr_size[i]);
            }
            return true;
        }
        else if(tp->ty==4)
        {
            auto ttp = (PtrEntry*) tp;
            for(int i = 1; i < ttp->arr_d; i++)
            {
                v.push_back(ttp->arr_size[i]);
            }
            return true;
        }
        return false;
    }
    void DeleteParam(string target)
    {
        table_size--;
        for(auto it = table.begin(); it!=table.end();it++)
        {
            auto tp = *it;
            if(tp->sym_name==target)
            {
                table.erase(it);
                return;
            }
        }
    }
    bool InsertConst(string symname, int val)
    {
        auto exist = Query(symname);
        if(exist==nullptr)
        {
            auto tp = new ConstEntry(symname,val);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            if(exist->is_param==true)
            {
                DeleteParam(symname);
                auto tp = new ConstEntry(symname,val);
                table.push_back(tp);
                table_size++;
                return true;
            }
            return false;
        }
    }
    bool InsertVar(string varname, string varsym)
    {
        auto exist = Query(varname);
        if(exist==nullptr)
        {
            auto tp = new VarEntry(varname,varsym);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            if(exist->is_param==true)
            {
                DeleteParam(varname);
                auto tp = new VarEntry(varname,varsym);
                table.push_back(tp);
                table_size++;
                return true;
            }
            return false;
        }
    }
    bool InsertFunc(string func_name,bool func_ty)
    {
        if(Query(func_name)==nullptr)
        {
            auto tp = new FuncEntry(func_name,func_ty);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            return false;
        }
    }
    bool InsertArr(string arr_name,string arr_sym,bool const_,int dime,vector<int> &size_)
    {
        auto exist = Query(arr_name);
        if(exist==nullptr)
        {
            auto tp = new ArrEntry(arr_name,arr_sym,const_,dime,size_);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else
        {
            if(exist->is_param==true)
            {
                DeleteParam(arr_name);
                auto tp = new ArrEntry(arr_name,arr_sym,const_,dime,size_);
                table.push_back(tp);
                table_size++;
                return true;
            }
            return false;
        }
    }
    bool InsertPtr(string ptr_name,string ptr_sym,int dime,vector<int> &size_,string alloc_)
    {
        auto exist = Query(ptr_name);
        if(exist==nullptr)
        {
            auto tp = new PtrEntry(ptr_name,ptr_sym,dime,size_,alloc_);
            table.push_back(tp);
            table_size++;
            return true;
        }
        else    //作为形参的指针在插入时不应该出现重名参数情况
        {
            return false;
        }
    }
    void Print()
    {
        cout<<"\nLayer "<<layer<<endl;
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
            else if(entry->ty==2)
            {
                cout<<"func: "<<entry->sym_name<<endl;
            }
            else if(entry->ty==3)
            {
                cout<<"arr: "<<entry->sym_name<<", ";
                ArrEntry *tp = (ArrEntry*)entry;
                cout<<"dim: "<<tp->arr_d<<", scala: ";
                for(int i = 0; i < tp->arr_d; i++)
                {
                    cout<<tp->arr_size[i]<<" ";
                }
                cout<<endl;
            }
            else
            {
                cout<<"ptr: "<<entry->sym_name<<", ";
                PtrEntry *tp = (PtrEntry*)entry;
                cout<<"dim: "<<tp->arr_d<<", scala: ";
                for(int i = 0; i < tp->arr_d; i++)
                {
                    cout<<tp->arr_size[i]<<" ";
                }
                cout<<endl;
            }
        }
        cout<<endl;
    }
    void PrintUp()
    {
        Symtab* cnttab = this;
        do
        {
            cnttab->Print();
            cnttab = cnttab->parent;
        }
        while(cnttab!=nullptr);
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
        while (cnttab!=nullptr);
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