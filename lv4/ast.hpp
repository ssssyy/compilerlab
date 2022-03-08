#include <iostream>
#include <cstring>
#include <cstdio>
#include <memory>
#include <stack>
#include <vector>
#include <unordered_map>
#include "def.hpp"
#include "symtab.hpp"

using namespace std;
extern string name_table[20];
extern int name_count;
extern int tpname_count;


class BaseAST{
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::string&) const = 0;  //输出AST
    virtual void dfs(std::string&, std::stack<std::string>&, Symtab* symtab) const = 0;   //生成IR
};

class CompUnitAST: public BaseAST{
public:
    unique_ptr<BaseAST> func_def;

    void Dump(string &ast) const override{
        cout<<"CompUnitAST { ";
        ast+="CompUnitAST { ";
        func_def->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    void dfs(string &ans, stack<string>& aststack, Symtab* symtab) const override{
        func_def->dfs(ans,aststack,symtab);    //暂时不用输出
    }
};

class FuncDefAST: public BaseAST{
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    void Dump(string &ast) const override{
        cout<<"FuncDefAST { ";
        ast+="FuncDefAST { ";
        func_type->Dump(ast);
        cout<<", "<<ident<<", ";
        ast+=", "+ident+", ";
        block->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    void dfs(string &ans, stack<string>& aststack,Symtab* symtab) const override
    {
        //cout<<"fun ";
        ans+="fun ";
        //cout<<"@"+ident+"(";
        ans+="@"+ident+"(";
        //cout<<"): ";
        ans+="): ";
        func_type->dfs(ans,aststack,symtab);
        block->dfs(ans,aststack,symtab);
    }
};

class FuncTypeAST: public BaseAST{
public:
    void Dump(string &ast) const override{
        cout<<"FuncTypeAST { "<<"int"<<" }";
        ast+="FuncTypeAST { int }";
    }
    void dfs(string &ans, stack<string>& aststack,Symtab* symtab) const override{
        //cout<<"i32";
        ans+="i32";
    }
};

class BlockAST: public BaseAST{
public:
    unique_ptr<BaseAST> block_item_arr;

    void Dump(string &ast) const override{
        cout<<"BlockAST { ";
        ast+="BlockAST { ";
        block_item_arr->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    void dfs(string &ans, stack<string>& aststack, Symtab* symtab) const override{
        ans+="{\n";
        //cout<<"\%"<<name_table[0]<<":\n";
        ans+="\%"+name_table[0]+":\n";
        block_item_arr->dfs(ans,aststack,symtab);
        ans+="}\n";
    }
};

class BlockItemArrAST: public BaseAST{
public:
    unique_ptr<BaseAST> block_item_arr;
    unique_ptr<BaseAST> block_item;

    void Dump(string &ast) const override{
        //我不想实现了，不调用就好了
        cout<<1;
    }
    void dfs(string &ans, stack<string>& aststack,Symtab* symtab) const override{
        if(block_item_arr!=nullptr)
        {
            block_item_arr->dfs(ans,aststack,symtab);
            block_item->dfs(ans,aststack,symtab);
        }
        else if(block_item!=nullptr)
        {
            block_item->dfs(ans,aststack,symtab);
        }
    }
};

class BlockItemAST: public BaseAST{
public:
    unique_ptr<BaseAST> stmt;
    unique_ptr<BaseAST> decl;

    void Dump(string &ast) const override{
        cout<<2;
    }
    void dfs(string &ans, stack<string>& aststack,Symtab* symtab) const override{
        if(stmt!=nullptr)
        {
            stmt->dfs(ans,aststack,symtab);
        }
        else
        {
            decl->dfs(ans,aststack,symtab);
        }
        genAST(ans,aststack,symtab);
    }
    void genAST(string &ans, stack<string>& aststack, Symtab* symtab) const 
    {
        stack<string> fuzhu;  //辅助栈，用来对后缀表达式求值
        while(!aststack.empty())
        {
            string val1,val2,val3;
            val1 = aststack.top();
            aststack.pop();
            if(val1=="(") continue;
            if(val1==")") continue;
            if(val1=="ret")
            {
                string tp1 = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(tp1);
                if(tp==nullptr)
                {
                    ans+="\tret "+tp1+"\n";
                    continue;
                }
                else    //符号名 需要load以后再返回
                {
                    string tpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp = (VarEntry*) tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\tret "+tpname+"\n";
                }
            }
            if(val1=="+ 1")
            {
                continue;
            }
            if(val1=="- 1")
            {
                string tpname = "%"+to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(ts);
                if(tp==nullptr)
                {
                    ans+="\t"+tpname+" = sub 0, "+ts+"\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry* ttp = (VarEntry*)tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub 0, "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if(val1=="! 1")
            {
                string tpname = "%"+to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(ts);
                if(tp==nullptr)
                {
                    ans+="\t"+tpname+" = eq "+ts+", 0\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry* ttp = (VarEntry*)tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+tpname+", 0\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if(val1=="+")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = add "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = add "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = add "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = add "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="-")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = sub "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = sub "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="*")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = mul "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mul "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mul "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = mul "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="/")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = div "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = div "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = div "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = div "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="%")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = mod "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mod "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mod "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = mod "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="<")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = lt "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = lt "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = lt "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = lt "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1==">")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = gt "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = gt "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = gt "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = gt "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="<=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = le "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = le "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = le "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = le "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1==">=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ge "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ge "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ge "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = ge "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="==")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = eq "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = eq "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="!=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ne "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ne "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ne "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = ne "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="&&")  
            {
                string tpname = "%"+to_string(tpname_count++);
                string ttpname = "%"+to_string(tpname_count++);
                string tttpname = "%"+to_string(tpname_count++);
                string ttttpname = "%"+to_string(tpname_count++);
                string tttttpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr&&tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ne 0, "+val2+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+val3+"\n";
                    ans+="\t"+tttpname+" = and "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+val3+"\n";
                    ans+="\t"+ttttpname+" = and "+ttpname+", "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+val2+"\n";
                    ans+="\t"+ttttpname+" = and "+ttpname+", "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+varsym1+"\n";
                    ans+="\t"+ttttpname+" = ne 0, "+varsym2+"\n";
                    ans+="\t"+tttttpname+" = and "+tttpname+", "+ttttpname+"\n";
                    fuzhu.push(tttttpname);
                }
                continue;
            }
            if(val1=="||")  
            {
                string tpname = "%"+to_string(tpname_count++);
                string ttpname = "%"+to_string(tpname_count++);
                string tttpname = "%"+to_string(tpname_count++);
                string ttttpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr&&tp2==nullptr)
                {
                    ans+="\t"+tpname+" = or "+val2+", "+val3+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = or "+varsym+", "+val3+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = or "+val2+", "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    ans+="\t"+tttpname+" = or "+varsym1+", "+varsym2+"\n";
                    ans+="\t"+ttttpname+" = ne 0, "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                continue;
            }
            if(val1=="=")
            {
                string rhs = fuzhu.top();
                fuzhu.pop();
                string lhs = aststack.top();
                aststack.pop();
                auto tp = symtab->Query(lhs);
                if(tp==nullptr)
                {
                    cerr<<"Error, not find the LVal "<<lhs<<endl;
                    exit(1);
                }
                VarEntry* ttp = (VarEntry*)tp;
                ans+="\tstore "+rhs+", "+ttp->var_sym+"\n";
                continue;
            }
            fuzhu.push(val1);    
        }
    }
};

class DeclAST: public BaseAST{
public:
    unique_ptr<BaseAST> const_decl;
    unique_ptr<BaseAST> var_decl;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(const_decl!=nullptr)
        {
            const_decl->dfs(ans,aststack,symtab);
        }
        else if(var_decl!=nullptr)
        {
            var_decl->dfs(ans,aststack,symtab);
        } 
    }
};

class VarDeclAST: public BaseAST{
public:
    unique_ptr<BaseAST> var_def_arr;

    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        var_def_arr->dfs(ans,aststack,symtab);
    }
};

class VarDefArrAST: public BaseAST{
public:
    unique_ptr<BaseAST> var_def_arr;
    unique_ptr<BaseAST> var_def;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(var_def_arr!=nullptr)
        { 
            var_def_arr->dfs(ans,aststack,symtab);
            var_def->dfs(ans,aststack,symtab);
        }
        else
        {
            var_def->dfs(ans,aststack,symtab);
        }
    }
};

class VarDefAST: public BaseAST{
public:
    unique_ptr<BaseAST> init_val;
    string ident;
    void Dump(string &ast) const override{
        cout<<3;
    }
    //这里应该有插入符号表的操作，TBC
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(init_val!=nullptr)
        { 
            aststack.push(ident);
            aststack.push("=");
            init_val->dfs(ans,aststack,symtab);
            symtab->InsertVar(ident,"@"+ident);
            ans+="\t@"+ident+" = alloc i32\n";
            genAST(ans,aststack,symtab);
        }
        else
        {
            symtab->InsertVar(ident,"@"+ident);
            ans+="\t@"+ident+" = alloc i32\n";
        }
        //symtab->Print();
    }
    void genAST(string &ans, stack<string>& aststack, Symtab* symtab) const 
    {
        stack<string> fuzhu;  //辅助栈，用来对后缀表达式求值
        while(!aststack.empty())
        {
            string val1,val2,val3;
            val1 = aststack.top();
            aststack.pop();
            if(val1=="(") continue;
            if(val1==")") continue;
            if(val1=="ret")
            {
                string tp1 = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(tp1);
                if(tp==nullptr)
                {
                    ans+="\tret "+tp1+"\n";
                    continue;
                }
                else    //符号名 需要load以后再返回
                {
                    string tpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp = (VarEntry*) tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\tret "+tpname+"\n";
                }
            }
            if(val1=="+ 1")
            {
                continue;
            }
            if(val1=="- 1")
            {
                string tpname = "%"+to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(ts);
                if(tp==nullptr)
                {
                    ans+="\t"+tpname+" = sub 0, "+ts+"\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry* ttp = (VarEntry*)tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub 0, "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if(val1=="! 1")
            {
                string tpname = "%"+to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->Query(ts);
                if(tp==nullptr)
                {
                    ans+="\t"+tpname+" = eq "+ts+", 0\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry* ttp = (VarEntry*)tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+tpname+", 0\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if(val1=="+")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = add "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = add "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = add "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = add "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="-")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = sub "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = sub "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = sub "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="*")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = mul "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mul "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mul "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = mul "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="/")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = div "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = div "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = div "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = div "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="%")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = mod "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mod "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = mod "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = mod "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="<")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = lt "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = lt "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = lt "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = lt "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1==">")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = gt "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = gt "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = gt "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = gt "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="<=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = le "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = le "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = le "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = le "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1==">=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ge "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ge "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ge "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = ge "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="==")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = eq "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = eq "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = eq "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="!=")
            {
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr && tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ne "+val2+", "+val3+"\n";
                    fuzhu.push(tpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ne "+tpname+", "+val3+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+ttpname+" = ne "+val2+", "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    string ttpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    string tttpname = "%"+to_string(tpname_count++);
                    ans+="\t"+tttpname+" = ne "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if(val1=="&&")  
            {
                string tpname = "%"+to_string(tpname_count++);
                string ttpname = "%"+to_string(tpname_count++);
                string tttpname = "%"+to_string(tpname_count++);
                string ttttpname = "%"+to_string(tpname_count++);
                string tttttpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr&&tp2==nullptr)
                {
                    ans+="\t"+tpname+" = ne 0, "+val2+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+val3+"\n";
                    ans+="\t"+tttpname+" = and "+tpname+", "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+val3+"\n";
                    ans+="\t"+ttttpname+" = and "+ttpname+", "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+val2+"\n";
                    ans+="\t"+ttttpname+" = and "+ttpname+", "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+varsym1+"\n";
                    ans+="\t"+ttttpname+" = ne 0, "+varsym2+"\n";
                    ans+="\t"+tttttpname+" = and "+tttpname+", "+ttttpname+"\n";
                    fuzhu.push(tttttpname);
                }
                continue;
            }
            if(val1=="||")  
            {
                string tpname = "%"+to_string(tpname_count++);
                string ttpname = "%"+to_string(tpname_count++);
                string tttpname = "%"+to_string(tpname_count++);
                string ttttpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->Query(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->Query(val3);
                if(tp1==nullptr&&tp2==nullptr)
                {
                    ans+="\t"+tpname+" = or "+val2+", "+val3+"\n";
                    ans+="\t"+ttpname+" = ne 0, "+tpname+"\n";
                    fuzhu.push(ttpname);
                }
                else if(tp1!=nullptr&&tp2==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp1;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = or "+varsym+", "+val3+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else if(tp2!=nullptr&&tp1==nullptr)
                {
                    VarEntry* ttp = (VarEntry*) tp2;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\t"+ttpname+" = or "+val2+", "+varsym+"\n";
                    ans+="\t"+tttpname+" = ne 0, "+ttpname+"\n";
                    fuzhu.push(tttpname);
                }
                else
                {
                    VarEntry* ttp1 = (VarEntry*) tp1;
                    string varsym1 = ttp1->var_sym;
                    ans+="\t"+tpname+" = load "+varsym1+"\n";
                    VarEntry* ttp2 = (VarEntry*) tp2;
                    string varsym2 = ttp2->var_sym;
                    ans+="\t"+ttpname+" = load "+varsym2+"\n";
                    ans+="\t"+tttpname+" = or "+varsym1+", "+varsym2+"\n";
                    ans+="\t"+ttttpname+" = ne 0, "+tttpname+"\n";
                    fuzhu.push(ttttpname);
                }
                continue;
            }
            if(val1=="=")
            {
                string rhs = fuzhu.top();
                fuzhu.pop();
                string lhs = aststack.top();
                aststack.pop();
                auto tp = symtab->Query(lhs);
                if(tp==nullptr)
                {
                    cerr<<"Error, not find the LVal "<<lhs<<endl;
                    exit(1);
                }
                VarEntry* ttp = (VarEntry*)tp;
                ans+="\tstore "+rhs+", "+ttp->var_sym+"\n";
                continue;
            }
            fuzhu.push(val1);    
        }
    }
};

class InitValAST: public BaseAST{
public:
    unique_ptr<BaseAST> exp;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        exp->dfs(ans,aststack,symtab);
    }
};

class ConstDeclAST: public BaseAST{
public:
    unique_ptr<BaseAST> const_def_arr;

    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        //cout<<"Const Decl"<<endl;
        const_def_arr->dfs(ans,aststack,symtab);
    }
};

class ConstDefArrAST: public BaseAST{
public:
    unique_ptr<BaseAST> const_def_arr;
    unique_ptr<BaseAST> const_def;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(const_def_arr!=nullptr)
        { 
            const_def_arr->dfs(ans,aststack,symtab);
            const_def->dfs(ans,aststack,symtab);
        }
        else
        {
            const_def->dfs(ans,aststack,symtab);
        }
    }
};

class ConstDefAST: public BaseAST{
public:
    unique_ptr<BaseAST> const_init_val;
    string ident;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        //添加符号表的操作，需要在constdefast里面进行符号表操作，并写出编译求值函数
        //cout<<"Const Def"<<endl;
        const_init_val->dfs(ans,aststack,symtab);
        //TBC
        int const_val = cal(aststack);
        symtab->InsertConst(ident,const_val);
    }
    int cal(stack<string>& aststack) const
    {
        //栈里面有+-!, +-*/%, 六个关系比较，|| &&
        //遇到操作符就弹出1/2个ans_stack里的数进行运算
        stack<int> ans_stack;
        while(!aststack.empty())
        {
            string tp = aststack.top();
            aststack.pop();
            if(tp=="+ 1")
            {
                continue;
            }
            else if(tp=="- 1")
            {
                int tn = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(-tn);
            }
            else if(tp=="! 1")
            {
                int tn = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(!tn);
            }
            else if(tp=="+")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1+tn2);
            }
            else if(tp=="-")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1-tn2);
            }
            else if(tp=="*")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1*tn2);
            }
            else if(tp=="/")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1/tn2);
            }
            else if(tp=="%")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1%tn2);
            }
            else if(tp=="<")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1<tn2);
            }
            else if(tp==">")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1>tn2);
            }
            else if(tp=="<=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1<=tn2);
            }
            else if(tp==">=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1>=tn2);
            }
            else if(tp=="==")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1==tn2);   
            }
            else if(tp=="!=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1!=tn2);
            }
            else if(tp=="&&")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1&&tn2);
            }
            else if(tp=="||")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1||tn2);
            }
            else    //数字，转换一下后存储
            {
                ans_stack.push(atoi(tp.c_str()));
            }
        }
        return ans_stack.top();
    }    
};

class ConstInitValAST: public BaseAST{
public:
    unique_ptr<BaseAST> const_exp;

    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        const_exp->dfs(ans,aststack,symtab);
    }    
};

class ConstExpAST: public BaseAST{
public:
    unique_ptr<BaseAST> exp;
    void Dump(string &ast) const override{
        cout<<3;
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        //cout<<"Const Exp"<<endl;
        exp->dfs(ans,aststack,symtab);
        //cout<<"Size2:"<<aststack.size()<<endl;
    }    
};

class StmtAST: public BaseAST{
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    string stmt;    //return

    void Dump(string& ast) const override{
        //废弃的函数，懒得改了
        cout<<"StmtAST { ";
        ast+="StmtAST { ";
        exp->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    //buggy
    void dfs(string &ans,stack<string>& aststack,Symtab* symtab) const override{
        if(lval!=nullptr)
        {
            lval->dfs(ans,aststack,symtab);
            aststack.push("=");
            exp->dfs(ans,aststack,symtab);
        }
        else
        {
            aststack.push("ret");
            exp->dfs(ans,aststack,symtab);
        }
    }
};

class ExpAST: public BaseAST{
public:
    unique_ptr<BaseAST> lor_exp;
    void Dump(string& ast) const override{
        cout<<"ExpAST { ";
        ast+="ExpAST { ";
        lor_exp->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    void dfs(string &ans,stack<string>& aststack,Symtab* symtab) const override{
        lor_exp->dfs(ans,aststack,symtab);
    }
};

class UnaryExpAST: public BaseAST{
public:
    unique_ptr<BaseAST> primary_exp;
    unique_ptr<BaseAST> unary_op;
    unique_ptr<BaseAST> unary_exp;
    void Dump(string& ast) const override{
        cout<<"UnaryExpAST { ";
        ast+="UnaryExpAST { ";
        if(primary_exp!=nullptr)    //生成了PrimaryExp单替换
        {
            primary_exp->Dump(ast);
            cout<<" }";
            ast+=" }";
        }
        else if(unary_op!=nullptr && unary_exp!=nullptr)
        {
            unary_op->Dump(ast);
            cout<<", ";
            ast+=", ";
            unary_exp->Dump(ast);
            cout<<" }";
            ast+=" }";
        }
    }
    void dfs(string &ans,stack<string>& aststack,Symtab* symtab) const override{
        if(primary_exp!=nullptr)
        {
            primary_exp->dfs(ans,aststack,symtab);
        }
        else if(unary_op!=nullptr && unary_exp!=nullptr)
        {
            unary_op->dfs(ans,aststack,symtab);
            unary_exp->dfs(ans,aststack,symtab);
        }
    }
};

class LValAST: public BaseAST{
public:
    string ident;

    void Dump(string &ast) const override{
        cout<<ident<<" ";
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        //查表的操作
        //cout<<ident<<endl;
        //symtab->Print();
        auto tp = symtab->Query(ident);
        if(tp==nullptr)
        {
            cerr<<"No declaration before"<<endl;
            exit(1);
        }
        int sym_type = tp->ty;
        if(sym_type==0)
        {
            ConstEntry* ttp = (ConstEntry*) tp;
            int val = ttp->const_val;
            aststack.push(to_string(val));
        }
        else
        {
            aststack.push(ident);
        }
        
    }
};

class PrimaryExpAST: public BaseAST{
public:
    int number;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;   
    void Dump(string& ast) const override{
        cout<<"PrimaryExpAST { ";
        ast+="PrimaryExpAST { ";
        if(exp!=nullptr)
        {
            cout<<"(";
            ast+="(";
            exp->Dump(ast);
            cout<<")";
            ast+=")";
        }
        else if(lval==nullptr)
        {
            cout<<number;
            ast+=to_string(number);
        }
        else
        {
            lval->Dump(ast);
        }
        cout<<" }";
        ast+=" }";
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(exp!=nullptr)
        {
            //cout<<"(";
            aststack.push("(");
            exp->dfs(ans,aststack,symtab);
            //cout<<")";
            aststack.push(")");
        }
        else if(lval==nullptr)
        {
            //cout<<number;
            aststack.push(to_string(number));
        }
        else
        {
            lval->dfs(ans,aststack,symtab);
        }
    }
};

class UnaryOpAST: public BaseAST{
public:
    string unary_op;  // + - !
    void Dump(string& ast) const override{
        cout<<"UnaryOpAST { ";
        ast+="UnaryOpAST { ";
        cout<<unary_op;
        ast+=unary_op;
        cout<<" }";
        ast+=" }";
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        //cout<<unary_op<<" ";
        aststack.push(unary_op+" 1");
    }
};

class AddExpAST: public BaseAST{
public:
    unique_ptr<BaseAST> mul_exp;
    unique_ptr<BaseAST> add_exp;
    string add_op; //+ -, binary op
    void Dump(string& ast) const override{
        cout<<"AddExpAST { ";
        ast+="AddExpAST { ";
        if(add_exp!=nullptr)
        {
            add_exp->Dump(ast);
            cout<<" "<<add_op<<" ";
            ast+=" "+add_op+" ";
            mul_exp->Dump(ast);
        }
        else
        {
            mul_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(add_exp!=nullptr)
        {
            aststack.push(add_op);
            //cout<<add_op<<" ";
            add_exp->dfs(ans,aststack,symtab);
            mul_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            mul_exp->dfs(ans,aststack,symtab);
        }
    }
};

class MulExpAST: public BaseAST{
public:
    unique_ptr<BaseAST> unary_exp;
    unique_ptr<BaseAST> mul_exp;
    string mul_op; //* / %, binary op
    void Dump(string& ast) const override{
        cout<<"MulExpAST { ";
        ast+="MulExpAST { ";
        if(mul_exp!=nullptr)
        {
            mul_exp->Dump(ast);
            cout<<" "<<mul_op<<" ";
            ast+=" "+mul_op+" ";
            unary_exp->Dump(ast);
        }
        else
        {
            unary_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(mul_exp!=nullptr)
        {
            aststack.push(mul_op);
            cout<<mul_op<<" ";
            mul_exp->dfs(ans,aststack,symtab);
            unary_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            unary_exp->dfs(ans,aststack,symtab);
        }
    }
};

class LOrExpAST: public BaseAST {
public:
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> lor_exp;
    void Dump(string& ast) const override{
        cout<<"LOrExpAST { ";
        ast+="LOrExpAST { ";
        if(lor_exp!=nullptr)
        {
            lor_exp->Dump(ast);
            cout<<" || ";
            ast+=" || ";
            land_exp->Dump(ast);
        }
        else
        {
            land_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(lor_exp!=nullptr)
        {
            aststack.push("||");
            //cout<<"||"<<" ";
            lor_exp->dfs(ans,aststack,symtab);
            land_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            land_exp->dfs(ans,aststack,symtab);
        }
    }
};

class LAndExpAST: public BaseAST {
public:
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> eq_exp;
    void Dump(string& ast) const override{
        cout<<"LAndExpAST { ";
        ast+="LAndExpAST { ";
        if(land_exp!=nullptr)
        {
            land_exp->Dump(ast);
            cout<<" && ";
            ast+=" && ";
            eq_exp->Dump(ast);
        }
        else
        {
            eq_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(land_exp!=nullptr)
        {
            aststack.push("&&");
            //cout<<"&&"<<" ";
            land_exp->dfs(ans,aststack,symtab);
            eq_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            eq_exp->dfs(ans,aststack,symtab);
        }
    }
};

class EqExpAST: public BaseAST {
public:
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> eq_exp;
    string eq_op;
    void Dump(string& ast) const override{
        cout<<"EqExpAST { ";
        ast+="EqExpAST { ";
        if(eq_exp!=nullptr)
        {
            eq_exp->Dump(ast);
            cout<<" "<<eq_op<<" ";
            ast+=" "+eq_op+" ";
            rel_exp->Dump(ast);
        }
        else
        {
            rel_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(eq_exp!=nullptr)
        {
            aststack.push(eq_op);
            //cout<<eq_op<<" ";
            eq_exp->dfs(ans,aststack,symtab);
            rel_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            rel_exp->dfs(ans,aststack,symtab);
        }
    }
};

class RelExpAST: public BaseAST {
public:
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> add_exp;
    string rel_op;
    void Dump(string& ast) const override{
        cout<<"RelExpAST { ";
        ast+="RelExpAST { ";
        if(rel_exp!=nullptr)
        {
            rel_exp->Dump(ast);
            cout<<" "<<rel_op<<" ";
            ast+=" "+rel_op+" ";
            add_exp->Dump(ast);
        }
        else
        {
            add_exp->Dump(ast);
        }
    }
    void dfs(string& ans, stack<string>& aststack,Symtab* symtab) const override{
        if(rel_exp!=nullptr)
        {
            aststack.push(rel_op);
            //cout<<rel_op<<" ";
            rel_exp->dfs(ans,aststack,symtab);
            add_exp->dfs(ans,aststack,symtab);
        }
        else
        {
            add_exp->dfs(ans,aststack,symtab);
        }
    }
};

