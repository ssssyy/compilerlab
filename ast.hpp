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
extern int if_count, else_count, then_count, while_count, while_not_count;
extern int current_while;
extern int shortcut_count;
extern int cal_val;
extern bool ret_in_branch;
extern bool is_ret_inside;
extern bool last_ret;
extern bool inside_if_else;
extern bool is_in_shortcut;
extern bool exp_is_const;
extern bool in_and;
extern stack<string> and_stack;
extern stack<string> or_stack;

class BaseAST{
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::string&) const = 0;  //输出AST
    virtual void dfs(std::string&, std::stack<std::string>&, Symtab* symtab) const = 0;   //生成IR
    void genAST(string &ans, stack<string>& aststack, Symtab* symtab) const 
    {
        stack<string> fuzhu;  //辅助栈，用来对后缀表达式求值
        /*
        stack<string> s1;
        while(!aststack.empty())
        {
            s1.push(aststack.top());
            cout<<aststack.top()<<" ";
            aststack.pop();
        }
        while(!s1.empty())
        {
            aststack.push(s1.top());
            s1.pop();
        }
        cout<<endl;
        */
        stack<int> valstack;
        while(!aststack.empty())
        {
            string val1,val2,val3;
            val1 = aststack.top();
            aststack.pop();
            if(val1=="(") continue;
            if(val1==")") continue;
            if(val1=="ret")
            {
                //cout<<"ret size: "<<aststack.size()<<endl;
                is_ret_inside = true;
                string tp1 = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->QueryUp(tp1);
                if(tp==nullptr)
                {
                    ans+="\tstore "+tp1+", \%ret\n";
                    ans+="\tjump \%end\n";     
                }
                else    //符号名 需要load以后再返回
                {
                    string tpname = "%"+to_string(tpname_count++);
                    VarEntry* ttp = (VarEntry*) tp;
                    string varsym = ttp->var_sym;
                    ans+="\t"+tpname+" = load "+varsym+"\n";
                    ans+="\tstore "+tpname+", \%ret\n";
                    ans+="\tjump \%end\n";
                }
                if(ret_in_branch==false&&inside_if_else==false)
                {
                    last_ret = true;
                }
                continue;
            }
            //buggy
            if(val1=="retvoid")
            {
                is_ret_inside = true;
                //ans+="\tret\n";
                ans+="\tjump \%end\n";
                if(ret_in_branch==false&&inside_if_else==false)
                {
                    last_ret = true;
                }
                continue;
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
                if(exp_is_const)
                {
                    int tpn = valstack.top();
                    valstack.pop();
                    valstack.push(-tpn);
                }
                auto tp = symtab->QueryUp(ts);
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
                if(exp_is_const)
                {
                    int tpn = valstack.top();
                    valstack.pop();
                    valstack.push(!tpn);
                }
                auto tp = symtab->QueryUp(ts);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1+tpn2);
                }
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1-tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1*tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1/tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1%tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1<tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1>tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1<=tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1>=tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1==tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
                if(exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1!=tpn2);
                }
                string tpname = "%"+to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
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
            if(val1=="=")
            {
                string rhs = fuzhu.top();
                fuzhu.pop();
                string lhs = aststack.top();
                aststack.pop();
                auto tp = symtab->QueryUp(lhs);
                if(tp==nullptr)
                {
                    cerr<<"Error, not find the LVal "<<lhs<<endl;
                    exit(1);
                }
                VarEntry* ttp = (VarEntry*)tp;
                auto tp2 = symtab->Query(rhs);
                if(tp2==nullptr)
                {
                    ans+="\tstore "+rhs+", "+ttp->var_sym+"\n";
                }
                else
                {
                    VarEntry* ttp2 = (VarEntry*)tp2;
                    string tpname = "\%"+to_string(tpname_count++);
                    ans+="\t"+tpname+" = load "+ttp2->var_sym+"\n";
                    ans+="\tstore "+tpname+", "+ttp->var_sym+"\n";
                }   
                continue;
            }
            fuzhu.push(val1);
            if(exp_is_const) valstack.push(atoi(val1.c_str()));
        }
        if(!fuzhu.empty())
        {
            aststack.push(fuzhu.top());
            fuzhu.pop();
        }
        if(exp_is_const) cal_val = valstack.top();
    }
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
        ans+="fun ";
        ans+="@"+ident+"(";
        ans+="): ";
        func_type->dfs(ans,aststack,symtab);
        ans+="{\n";
        ans+="\%"+name_table[name_count++]+":\n";
        //以后可能需要特判是否有返回值
        ans+="\t%ret = alloc i32\n";
        ans+="\tjump %mystart\n";
        ans+="%mystart:\n";
        block->dfs(ans,aststack,symtab);
        //需要考虑前面有没有分支跳转 buggy
        ans+="\tjump \%end\n";
        ans+="\%end:\n";
        string last_sym = "\%"+to_string(tpname_count++);
        ans+="\t"+last_sym+" = load %ret\n";
        ans+="\tret "+last_sym+"\n";
        ans+="}\n";
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
        Symtab* cnt = symtab->NewSymTab();
        block_item_arr->dfs(ans,aststack,cnt);
        cnt->DeleteSymTab();
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
        last_ret = false;
        genAST(ans,aststack,symtab);
        if(last_ret==true)
        {
            string bbname = "\%"+name_table[name_count++];
            ans+=bbname+":\n";
        }
        
        while(!aststack.empty()) aststack.pop();
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
            symtab->InsertVar(ident,"@"+ident+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no));
            ans+="\t@"+ident+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+" = alloc i32\n";
            last_ret = false;
            genAST(ans,aststack,symtab);
        }
        else
        {
            symtab->InsertVar(ident,"@"+ident+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no));
            ans+="\t@"+ident+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+" = alloc i32\n";
        }
        //symtab->Print();
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
        exp_is_const = true;
        cal_val = 0;
        const_init_val->dfs(ans,aststack,symtab);
        //TBC
        int const_val = cal(aststack,symtab);
        symtab->InsertConst(ident,const_val);
        exp_is_const = false;
    }
    int cal(stack<string>& aststack,Symtab* symtab) const
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
            else if(tp[0]>='0'&&tp[0]<='9')    //数字，转换一下后存储
            {
                ans_stack.push(atoi(tp.c_str()));
            }
            else
            {
                auto tpp = symtab->QueryUp(tp);
                string tpname = "%"+to_string(tpname_count++);
                VarEntry* ttp = (VarEntry*) tpp;
                int val = ttp->val;
                ans_stack.push(val);
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
    unique_ptr<BaseAST> block;
    unique_ptr<BaseAST> condition;
    unique_ptr<BaseAST> stmt1;
    unique_ptr<BaseAST> stmt2;
    string stmt;    //return,if,ifelse,while

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
        else if(block!=nullptr)
        {
            block->dfs(ans,aststack,symtab);
        }
        else if(exp!=nullptr&&stmt=="return")
        {
            aststack.push("ret");
            exp->dfs(ans,aststack,symtab);
        }
        else if(exp!=nullptr&&stmt=="while")
        {
            string entry_name = "\%while_entry_"+to_string(while_count);
            string body_name = "\%while_body_"+to_string(while_count);
            string end_name = "\%while_end_"+to_string(while_count++);
            ans+="\tjump "+entry_name+"\n";
            ans+=entry_name+":\n";
            exp->dfs(ans,aststack,symtab);
            genAST(ans,aststack,symtab);
            string r_name = aststack.top();
            aststack.pop();
            if(r_name[0]=='%'||isdigit(r_name[0]))
            {
                ans+="\tbr "+r_name+", "+body_name+", "+end_name+"\n";
            }
            else
            { 
                auto tp = symtab->QueryUp(r_name);
                VarEntry* ttp = (VarEntry*)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans+="\t%"+tpname+" = load "+varsym+"\n";
                ans+="\tbr %"+tpname+", "+body_name+", "+end_name+"\n";
            }
            ans+=body_name+":\n";
            auto ntable = symtab->NewSymTab();
            current_while++;
            stmt1->dfs(ans,aststack,ntable);
            is_ret_inside = false;
            genAST(ans,aststack,symtab);
            ntable->DeleteSymTab();
            current_while--;
            while(!aststack.empty()) aststack.pop();
            if(!is_ret_inside) ans+="\tjump "+entry_name+"\n";
            is_ret_inside = false;
            ans+=end_name+":\n";
        }
        else if(exp!=nullptr)
        {
            exp->dfs(ans,aststack,symtab);
        }
        else if(stmt=="break")
        {
            string end_name = "\%while_end_"+to_string(current_while);
            ans+="\tjump "+end_name+"\n";
            ans+="\%while_not_"+to_string(while_not_count++)+":\n";
        }
        else if(stmt=="continue")
        {
            string entry_name = "\%while_entry_"+to_string(current_while);
            ans+="\tjump "+entry_name+"\n";
            ans+="\%while_not_"+to_string(while_not_count++)+":\n";
        }
        else if(stmt=="return")
        {
            aststack.push("retvoid");
        }
        else if(stmt=="if") //if (cond) stmt1
        {
            is_ret_inside = false;
            bool tpflag = false;
            if(inside_if_else==true) tpflag = true;
            else
            {
                inside_if_else=true;
                tpflag = false;
            }
            condition->dfs(ans,aststack,symtab);
            last_ret = false;
            genAST(ans,aststack,symtab);
            string r_name = aststack.top();
            aststack.pop();
            string ifname = "\%if_"+to_string(if_count++);
            string thenname = "\%then_"+to_string(then_count++);
            if(r_name[0]=='%'||isdigit(r_name[0]))
            {
                ans+="\tbr "+r_name+", "+ifname+", "+thenname+"\n";
            }
            else
            { 
                auto tp = symtab->QueryUp(r_name);
                VarEntry* ttp = (VarEntry*)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans+="\t%"+tpname+" = load "+varsym+"\n";
                ans+="\tbr %"+tpname+", "+ifname+", "+thenname+"\n";
            }
            ans+=ifname+":\n";
            auto ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt1->dfs(ans,aststack,ntable);
            //cout<<aststack.size()<<endl;
            last_ret = false;
            genAST(ans,aststack,ntable);
            ret_in_branch = false;
            while(!aststack.empty()) aststack.pop();
            ntable->DeleteSymTab();
            if(!is_ret_inside) ans+="\tjump "+thenname+"\n";
            is_ret_inside = false;
            ans+=thenname+":\n";
            if(!tpflag) inside_if_else = false;
        }
        else if(stmt=="ifelse") //if (cond) stmt1 else stmt2
        {
            is_ret_inside = false;
            bool tpflag = false;
            if(inside_if_else==true) tpflag = true;
            else
            {
                inside_if_else=true;
                tpflag = false;
            }
            condition->dfs(ans,aststack,symtab);
            last_ret = false;
            genAST(ans,aststack,symtab);
            string r_name = aststack.top();
            aststack.pop();
            string ifname = "\%if_"+to_string(if_count++);
            string thenname = "\%then_"+to_string(then_count++);
            string elsename = "\%else_"+to_string(else_count++);
            if(r_name[0]=='%'||isdigit(r_name[0]))
            {
                ans+="\tbr "+r_name+", "+ifname+", "+elsename+"\n";
            }
            else
            { 
                auto tp = symtab->QueryUp(r_name);
                VarEntry* ttp = (VarEntry*)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans+="\t%"+tpname+" = load "+varsym+"\n";
                ans+="\tbr %"+tpname+", "+ifname+", "+elsename+"\n";
            }
            ans+=ifname+":\n";
            auto ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt1->dfs(ans,aststack,ntable);
            last_ret = false;
            genAST(ans,aststack,ntable);
            ret_in_branch = false;
            while(!aststack.empty()) aststack.pop();
            ntable->DeleteSymTab();
            if(!is_ret_inside) ans+="\tjump "+thenname+"\n";
            ans+=elsename+":\n";
            is_ret_inside = false;
            ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt2->dfs(ans,aststack,ntable);
            last_ret = false;
            genAST(ans,aststack,ntable);
            ret_in_branch = false;
            while(!aststack.empty()) aststack.pop();
            ntable->DeleteSymTab();
            if(!is_ret_inside)
            {   
                ans+="\tjump "+thenname+"\n";
            }
            ans+=thenname+":\n";
            is_ret_inside = false;
            if(!tpflag) inside_if_else = false;
        }
        else
        {
            cout<<"";    //make compiler happy
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
        //cout<<1<<ident<<endl;
        auto tp = symtab->QueryUp(ident);
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
            bool flag = false;
            bool flag2 = false;
            if(is_in_shortcut==false)
            {
                //cout<<"or1"<<endl;
                flag = true;
                in_and = false;
                is_in_shortcut = true;
                string start_name = "\%tp_then_"+to_string(shortcut_count);
                or_stack.push(start_name);
            }
            else if(in_and==true)
            {
                //cout<<"or2"<<endl;
                flag2 = true;
                in_and = false;
                string tpname = "\%tp_then_"+to_string(shortcut_count);
                or_stack.push(tpname);
            }
            string result_name = "result_"+to_string(shortcut_count);
            string if_name = "\%tp_if_"+to_string(shortcut_count);
            string then_name = "\%tp_then_"+to_string(shortcut_count++);
            symtab->InsertVar(result_name,"@"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no));
            ans+="\t@"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+" = alloc i32\n";
            ans+="\tstore 1, @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
            stack<string> fuzhu;    //存一下当前栈里有的东西，然后算完短路求值以后再塞回去
            while(!aststack.empty())
            {
                fuzhu.push(aststack.top());
                aststack.pop();
            }

            aststack.push("==");
            lor_exp->dfs(ans,aststack,symtab);
            aststack.push("0");
            genAST(ans,aststack,symtab);
            string r_name = aststack.top();
            aststack.pop();
            //cout<<"Orsize: "<<or_stack.size()<<endl;
            if(r_name[0]=='%'||isdigit(r_name[0]))
            {
                ans+="\tbr "+r_name+", "+if_name+", "+or_stack.top()+"\n";
            }
            else
            { 
                auto tp = symtab->QueryUp(r_name);
                VarEntry* ttp = (VarEntry*)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans+="\t%"+tpname+" = load "+varsym+"\n";
                ans+="\tbr %"+tpname+", "+if_name+", "+or_stack.top()+"\n";
            }
            ans+=if_name+":\n";

            aststack.push("!=");
            land_exp->dfs(ans,aststack,symtab);
            aststack.push("0");
            genAST(ans,aststack,symtab);
            r_name = aststack.top();
            aststack.pop();
            auto tp = symtab->QueryUp(r_name);
            if(tp==nullptr)
            {
                ans+="\tstore "+r_name+", @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
                ans+="\tjump "+then_name+"\n";     
            }
            else    //符号名 需要load以后再返回
            {
                string tpname = "%"+to_string(tpname_count++);
                VarEntry* ttp = (VarEntry*) tp;
                string varsym = ttp->var_sym;
                ans+="\t"+tpname+" = load "+varsym+"\n";
                ans+="\tstore "+tpname+", @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
                ans+="\tjump "+then_name+"\n";
            }
            ans+=then_name+":\n";

            while(!fuzhu.empty())
            {
                aststack.push(fuzhu.top());
                fuzhu.pop();
            }
            if(flag2)
            {
                or_stack.pop();
                in_and = true;
            }
            if(exp_is_const==false) aststack.push(result_name);
            else aststack.push(to_string(cal_val));
            //symtab->Print();
            if(flag)
            {
                is_in_shortcut = false;
                or_stack.pop();
            }
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
            bool flag = false;
            bool flag2 = false;
            if(is_in_shortcut==false)
            {
                //cout<<"and1"<<endl;
                flag = true;
                in_and =true;
                is_in_shortcut = true;
                string start_name = "\%tp_then_"+to_string(shortcut_count);
                and_stack.push(start_name);
            }
            else if(in_and==false)
            {
                //cout<<"and2"<<endl;
                flag2 = true;
                in_and = true;
                string tpname = "\%tp_then_"+to_string(shortcut_count);
                and_stack.push(tpname);
            }
            string result_name = "result_"+to_string(shortcut_count);
            string if_name = "\%tp_if_"+to_string(shortcut_count);
            string then_name = "\%tp_then_"+to_string(shortcut_count++);
            symtab->InsertVar(result_name,"@"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no));
            ans+="\t@"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+" = alloc i32\n";
            ans+="\tstore 0, @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
            stack<string> fuzhu;    //存一下当前栈里有的东西，然后算完短路求值以后再塞回去
            while(!aststack.empty())
            {
                fuzhu.push(aststack.top());
                aststack.pop();
            }

            aststack.push("!=");
            land_exp->dfs(ans,aststack,symtab);
            aststack.push("0");
            genAST(ans,aststack,symtab);
            string r_name = aststack.top();
            aststack.pop();
            //cout<<"Andsize: "<<and_stack.size()<<endl;
            if(r_name[0]=='%'||isdigit(r_name[0]))
            {
                ans+="\tbr "+r_name+", "+if_name+", "+and_stack.top()+"\n";
            }
            else
            { 
                auto tp = symtab->QueryUp(r_name);
                VarEntry* ttp = (VarEntry*)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans+="\t%"+tpname+" = load "+varsym+"\n";
                ans+="\tbr %"+tpname+", "+if_name+", "+and_stack.top()+"\n";
            }
            ans+=if_name+":\n";

            aststack.push("!=");
            eq_exp->dfs(ans,aststack,symtab);
            aststack.push("0");
            genAST(ans,aststack,symtab);
            r_name = aststack.top();
            aststack.pop();
            auto tp = symtab->QueryUp(r_name);
            if(tp==nullptr)
            {
                ans+="\tstore "+r_name+", @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
                ans+="\tjump "+then_name+"\n";     
            }
            else    //符号名 需要load以后再返回
            {
                string tpname = "%"+to_string(tpname_count++);
                VarEntry* ttp = (VarEntry*) tp;
                string varsym = ttp->var_sym;
                ans+="\t"+tpname+" = load "+varsym+"\n";
                ans+="\tstore "+tpname+", @"+result_name+"_"+to_string(symtab->layer)+"_"+to_string(symtab->child_no)+"\n";
                ans+="\tjump "+then_name+"\n";
            }
            ans+=then_name+":\n";

            while(!fuzhu.empty())
            {
                aststack.push(fuzhu.top());
                fuzhu.pop();
            }
            if(flag2)
            {
                and_stack.pop();
                in_and = false;
            }
            if(exp_is_const==false) aststack.push(result_name);
            else aststack.push(to_string(cal_val));
            //symtab->Print();
            if(flag)
            {
                is_in_shortcut = false;
                and_stack.pop();
            }
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
            //cout<<" "<<eq_op<<" ";
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

class ConditionAST: public BaseAST{
public:
    public:
    unique_ptr<BaseAST> lor_exp;
    void Dump(string& ast) const override{
        cout<<"ConditionAST { ";
        ast+="ConditionAST { ";
        lor_exp->Dump(ast);
        cout<<" }";
        ast+=" }";
    }
    void dfs(string &ans,stack<string>& aststack,Symtab* symtab) const override{
        lor_exp->dfs(ans,aststack,symtab);
    }

};