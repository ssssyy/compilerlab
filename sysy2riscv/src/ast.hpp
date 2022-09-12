#include <iostream>
#include <cstring>
#include <cstdio>
#include <memory>
#include <stack>
#include <queue>
#include <vector>
#include <unordered_map>
#include "def.hpp"
#include "symtab.hpp"

using namespace std;
extern int name_count;
extern int tpname_count;
extern int if_count, else_count, then_count, while_count, while_not_count;
extern int current_while;
extern bool outer_while;
extern int shortcut_count;
extern int cal_val;
extern int mystart_count;
extern bool is_void_func;
extern bool ret_in_branch;
extern bool is_ret_inside;
extern bool last_ret;
extern bool inside_if_else;
extern bool is_in_shortcut;
extern bool exp_is_const;
extern bool in_and;
extern bool is_arr;
extern stack<int> and_stack_1;
extern stack<int> or_stack_1;
extern stack<string> and_stack;
extern stack<string> or_stack;
extern stack<string> const_val_stack;
extern queue<string> tp_params_sym;
extern bool is_cal_param;
extern Symtab *cnt_func_symtab;
extern bool use_cnt_tab;
extern int or_and_layer;
extern vector<int> arr_param;
extern string str_arr_init;
extern vector<string> arr_init_table;
extern int arr_layer;
extern int arr_total;
extern bool arr_is_const;
extern vector<string> arr_lval_param;
extern vector<int> arr_lval_param_layer;
extern bool param_is_arr;
extern int arr_param_layer;

class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::string &) const = 0;                                           //输出AST
    virtual void dfs(std::string &, std::stack<std::string> &, Symtab *symtab) const = 0; //生成IR
    void genAST(string &ans, stack<string> &aststack, Symtab *symtab) const
    {
        stack<string> fuzhu; //辅助栈，用来对后缀表达式求值
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
        while (!aststack.empty())
        {
            string val1, val2, val3;
            val1 = aststack.top();
            aststack.pop();
            if (val1 == "(")
                continue;
            if (val1 == ")")
                continue;
            if (val1 == "ret")
            {
                // cout<<"ret size: "<<aststack.size()<<endl;
                is_ret_inside = true;
                string tp1 = fuzhu.top();
                fuzhu.pop();
                auto tp = symtab->QueryUp(tp1);
                if (tp == nullptr)
                {
                    if (tp1[0] == '%' && (tp1[1] == 'p' || tp1[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + tp1 + "\n";
                        cout << "\t" << tpname << " = load " << tp1 << endl;
                        tp1 = tpname;
                    }
                    ans += "\tstore " + tp1 + ", \%ret\n";
                    string endname = "\%end" + to_string(mystart_count - 1);
                    ans += "\tjump " + endname + "\n";
                }
                else //符号名 需要load以后再返回
                {
                    string tpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp = (VarEntry *)tp;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    ans += "\tstore " + tpname + ", \%ret\n";
                    string endname = "\%end" + to_string(mystart_count - 1);
                    ans += "\tjump " + endname + "\n";
                }
                if (ret_in_branch == false && inside_if_else == false)
                {
                    last_ret = true;
                }
                continue;
            }
            // buggy
            if (val1 == "retvoid")
            {
                is_ret_inside = true;
                string endname = "\%end" + to_string(mystart_count - 1);
                ans += "\tjump " + endname + "\n";
                if (ret_in_branch == false && inside_if_else == false)
                {
                    last_ret = true;
                }
                continue;
            }
            if (val1 == "+ 1")
            {
                continue;
            }
            if (val1 == "- 1")
            {
                string tpname = "%" + to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                if (exp_is_const)
                {
                    int tpn = valstack.top();
                    valstack.pop();
                    valstack.push(-tpn);
                }
                auto tp = symtab->QueryUp(ts);
                if (tp == nullptr)
                {
                    if (ts[0] == '%' && (ts[1] == 'p' || ts[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + ts + "\n";
                        cout << "\t" << tpname << " = load " << ts << endl;
                        ts = tpname;
                    }
                    ans += "\t" + tpname + " = sub 0, " + ts + "\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry *ttp = (VarEntry *)tp;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = sub 0, " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if (val1 == "! 1")
            {
                string tpname = "%" + to_string(tpname_count++);
                string ts = fuzhu.top();
                fuzhu.pop();
                if (exp_is_const)
                {
                    int tpn = valstack.top();
                    valstack.pop();
                    valstack.push(!tpn);
                }
                auto tp = symtab->QueryUp(ts);
                if (tp == nullptr)
                {
                    if (ts[0] == '%' && (ts[1] == 'p' || ts[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + ts + "\n";
                        cout << "\t" << tpname << " = load " << ts << endl;
                        ts = tpname;
                    }
                    ans += "\t" + tpname + " = eq " + ts + ", 0\n";
                    fuzhu.push(tpname);
                }
                else
                {
                    VarEntry *ttp = (VarEntry *)tp;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = eq " + tpname + ", 0\n";
                    fuzhu.push(ttpname);
                }
                continue;
            }
            if (val1 == "+")
            {
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 + tpn2);
                }
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = add " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = add " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = add " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = add " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "-")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 - tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = sub " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = sub " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = sub " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = sub " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "*")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 * tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = mul " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = mul " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = mul " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = mul " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "/")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 / tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = div " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = div " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = div " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = div " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "%")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 % tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = mod " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = mod " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = mod " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = mod " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "<")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 < tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = lt " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = lt " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = lt " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = lt " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == ">")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 > tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = gt " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = gt " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = gt " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = gt " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "<=")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 <= tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = le " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = le " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = le " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = le " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == ">=")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 >= tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = ge " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = ge " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = ge " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = ge " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "==")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 == tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = eq " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = eq " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = eq " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = eq " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "!=")
            {
                if (exp_is_const)
                {
                    int tpn1 = valstack.top();
                    valstack.pop();
                    int tpn2 = valstack.top();
                    valstack.pop();
                    valstack.push(tpn1 != tpn2);
                }
                string tpname = "%" + to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                auto tp1 = symtab->QueryUp(val2);
                val3 = fuzhu.top();
                fuzhu.pop();
                auto tp2 = symtab->QueryUp(val3);
                if (tp1 == nullptr && tp2 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    ans += "\t" + tpname + " = ne " + val2 + ", " + val3 + "\n";
                    fuzhu.push(tpname);
                }
                else if (tp1 != nullptr && tp2 == nullptr)
                {
                    if (val3[0] == '%' && (val3[1] == 'p' || val3[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val3 + "\n";
                        cout << "\t" << tpname << " = load " << val3 << endl;
                        val3 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp1;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = ne " + tpname + ", " + val3 + "\n";
                    fuzhu.push(ttpname);
                }
                else if (tp2 != nullptr && tp1 == nullptr)
                {
                    if (val2[0] == '%' && (val2[1] == 'p' || val2[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + val2 + "\n";
                        cout << "\t" << tpname << " = load " << val2 << endl;
                        val2 = tpname;
                    }
                    VarEntry *ttp = (VarEntry *)tp2;
                    string varsym = ttp->var_sym;
                    ans += "\t" + tpname + " = load " + varsym + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = ne " + val2 + ", " + tpname + "\n";
                    fuzhu.push(ttpname);
                }
                else
                {
                    VarEntry *ttp1 = (VarEntry *)tp1;
                    string varsym1 = ttp1->var_sym;
                    ans += "\t" + tpname + " = load " + varsym1 + "\n";
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string varsym2 = ttp2->var_sym;
                    ans += "\t" + ttpname + " = load " + varsym2 + "\n";
                    string tttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + tttpname + " = ne " + tpname + ", " + ttpname + "\n";
                    fuzhu.push(tttpname);
                }
                continue;
            }
            if (val1 == "=")
            {
                string rhs = fuzhu.top();
                fuzhu.pop();
                string lhs = aststack.top();
                aststack.pop();
                string tar;
                if (lhs[0] == '%')
                {
                    tar = lhs;
                }
                else
                {
                    auto tp = symtab->QueryUp(lhs);
                    if (tp == nullptr)
                    {
                        cerr << "Error, not find the LVal " << lhs << endl;
                        exit(1);
                    }
                    VarEntry *ttp = (VarEntry *)tp;
                    tar = ttp->var_sym;
                }
                // cout<<rhs<<endl;
                auto tp2 = symtab->QueryUp(rhs);
                if (tp2 == nullptr)
                {
                    if (rhs[0] == '%' && (rhs[1] == 'p' || rhs[1] == 'q'))
                    {
                        string tpname = "%" + to_string(tpname_count++);
                        ans += "\t" + tpname + " = load " + rhs + "\n";
                        cout << "\t" << tpname << " = load " << rhs << endl;
                        rhs = tpname;
                    }
                    ans += "\tstore " + rhs + ", " + tar + "\n";
                }
                else
                {
                    VarEntry *ttp2 = (VarEntry *)tp2;
                    string tpname = "\%" + to_string(tpname_count++);
                    ans += "\t" + tpname + " = load " + ttp2->var_sym + "\n";
                    ans += "\tstore " + tpname + ", " + tar + "\n";
                }
                continue;
            }
            fuzhu.push(val1);
            if (exp_is_const)
                valstack.push(atoi(val1.c_str()));
        }
        if (!fuzhu.empty())
        {
            aststack.push(fuzhu.top());
            fuzhu.pop();
        }
        if (exp_is_const)
            cal_val = valstack.top();
    }
    int cal(stack<string> &aststack, Symtab *symtab) const
    {
        //栈里面有+-!, +-*/%, 六个关系比较，|| &&
        //遇到操作符就弹出1/2个ans_stack里的数进行运算
        stack<int> ans_stack;
        /*
        stack<string> fuzhu;
        while(!aststack.empty())
        {
            cout<<aststack.top()<<" ";
            fuzhu.push(aststack.top());
            aststack.pop();
        }
        while(!fuzhu.empty())
        {
            aststack.push(fuzhu.top());
            fuzhu.pop();
        }
        cout<<endl;
        */
       PrintStack(aststack);
        while (!aststack.empty())
        {
            string tp = aststack.top();
            aststack.pop();
            if (tp == "+ 1")
            {
                continue;
            }
            else if (tp == "(")
            {
                continue;
            }
            else if (tp == ")")
            {
                continue;
            }
            else if (tp == "- 1")
            {
                int tn = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(-tn);
            }
            else if (tp == "! 1")
            {
                int tn = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(!tn);
            }
            else if (tp == "+")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 + tn2);
            }
            else if (tp == "-")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 - tn2);
            }
            else if (tp == "*")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 * tn2);
            }
            else if (tp == "/")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 / tn2);
            }
            else if (tp == "%")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 % tn2);
            }
            else if (tp == "<")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 < tn2);
            }
            else if (tp == ">")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 > tn2);
            }
            else if (tp == "<=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 <= tn2);
            }
            else if (tp == ">=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 >= tn2);
            }
            else if (tp == "==")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 == tn2);
            }
            else if (tp == "!=")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 != tn2);
            }
            else if (tp == "&&")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 && tn2);
            }
            else if (tp == "||")
            {
                int tn1, tn2;
                tn1 = ans_stack.top();
                ans_stack.pop();
                tn2 = ans_stack.top();
                ans_stack.pop();
                ans_stack.push(tn1 || tn2);
            }
            else if (tp[0] >= '0' && tp[0] <= '9') //数字，转换一下后存储
            {
                ans_stack.push(atoi(tp.c_str()));
            }
            else
            {
                auto tpp = symtab->QueryUp(tp);
                string tpname = "%" + to_string(tpname_count++);
                VarEntry *ttp = (VarEntry *)tpp;
                int val = ttp->val;
                ans_stack.push(val);
            }
        }
        return ans_stack.top();
    }
    void PrintStack(stack<string> &aststack) const
    {
        stack<string> s1;
        while (!aststack.empty())
        {
            s1.push(aststack.top());
            cout << aststack.top() << " ";
            aststack.pop();
        }
        while (!s1.empty())
        {
            aststack.push(s1.top());
            s1.pop();
        }
        cout << endl;
    }
};

class FileUnitAST : public BaseAST
{
public:
    unique_ptr<BaseAST> comp_unit;
    void Dump(string &ast) const override
    {
        cout << 1 << endl;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // cout<<"FileUnitAST"<<endl;
        genLib(ans, symtab);
        comp_unit->dfs(ans, aststack, symtab);
        // symtab->Print();
    }
    void genLib(string &ans, Symtab *symtab) const
    {
        ans += "decl @getint(): i32\n";
        ans += "decl @getch(): i32\n";
        ans += "decl @getarray(*i32): i32\n";
        ans += "decl @putint(i32)\n";
        ans += "decl @putch(i32)\n";
        ans += "decl @putarray(i32, *i32)\n";
        ans += "decl @starttime()\n";
        ans += "decl @stoptime()\n";
        symtab->InsertFunc("getint", false);
        symtab->InsertFunc("getch", false);
        symtab->InsertFunc("getarray", false);
        symtab->InsertFunc("putint", true);
        symtab->InsertFunc("putch", true);
        symtab->InsertFunc("putarray", true);
        symtab->InsertFunc("starttime", true);
        symtab->InsertFunc("stoptime", true);
        ans += "\n";
    }
};

class CompUnitAST : public BaseAST
{
public:
    unique_ptr<BaseAST> func_def;
    unique_ptr<BaseAST> comp_unit;
    unique_ptr<BaseAST> decl;

    void Dump(string &ast) const override
    {
        cout << 1 << endl;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // cout<<"CompUnitAST"<<endl;
        if (comp_unit != nullptr && func_def != nullptr)
        {
            // cout<<"C1"<<endl;
            comp_unit->dfs(ans, aststack, symtab);
            ans += "\n";
            func_def->dfs(ans, aststack, symtab);
        }
        else if (comp_unit == nullptr && func_def != nullptr)
        {
            // cout<<"C2"<<endl;
            func_def->dfs(ans, aststack, symtab);
            ans += "\n";
        }
        else if (comp_unit != nullptr && decl != nullptr)
        {
            comp_unit->dfs(ans, aststack, symtab);
            decl->dfs(ans, aststack, symtab);
        }
        else
        {
            decl->dfs(ans, aststack, symtab);
        }
    }
};

class FuncDefAST : public BaseAST
{
public:
    string func_type;
    // unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> func_f_params;
    unique_ptr<BaseAST> block;

    void Dump(string &ast) const override
    {
        cout << 1;
    }
    //这里需要将函数插入全局符号表（函数类），待续
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // symtab->Print();
        // cout<<"FuncDefAST"<<endl;
        if (func_f_params == nullptr)
        {
            ans += "fun ";
            ans += "@" + ident + "(";
            ans += ")";
            cout << "fun @" << ident << "()";
            // func_type->dfs(ans, aststack, symtab);
            if (func_type == "int")
            {
                ans += ": i32";
                cout << ": i32";
                is_void_func = false;
            }
            else
            {
                is_void_func = true;
            }
            symtab->InsertFunc(ident, is_void_func);
            ans += "{\n";
            string namename = "\%e" + to_string(name_count++);
            ans += namename + ":\n";
            cout << "{\n"
                 << namename << ":\n";
            if (is_void_func == false)
            {
                // cout<<111<<endl;
                ans += "\t%ret = alloc i32\n";
                string endname = "\%end" + to_string(mystart_count);
                string mystartname = "\%mystart" + to_string(mystart_count++);
                ans += "\tjump " + mystartname + "\n";
                ans += mystartname + ":\n";
                cout << "\t%ret = alloc i32\n\tjump " + mystartname + "\n" + mystartname + ":\n";
                block->dfs(ans, aststack, symtab);
                //需要考虑前面有没有分支跳转 buggy
                ans += "\tjump " + endname + "\n";
                ans += endname + ":\n";
                string last_sym = "\%" + to_string(tpname_count++);
                ans += "\t" + last_sym + " = load %ret\n";
                ans += "\tret " + last_sym + "\n";
                ans += "}\n";
                cout << "\tjump " << endname << "\n"
                     << endname << ":\n\t" << last_sym << " = load %ret\n\tret " << last_sym << "\n}\n";
            }
            else
            {
                // cout<<222<<endl;
                string endname = "\%end" + to_string(mystart_count);
                string mystartname = "\%mystart" + to_string(mystart_count++);
                ans += "\tjump " + mystartname + "\n";
                ans += mystartname + ":\n";
                cout << "\tjump " << mystartname << "\n"
                     << mystartname << ":\n";
                block->dfs(ans, aststack, symtab);
                //需要考虑前面有没有分支跳转 buggy
                ans += "\tjump " + endname + "\n";
                ans += endname + ":\n";
                ans += "\tret\n";
                ans += "}\n";
                cout << "\tjump " << endname << "\n"
                     << endname << ":\n\tret\n}\n";
            }
        }
        else
        {
            use_cnt_tab = true;
            ans += "fun ";
            ans += "@" + ident + "(";
            cout << "fun @" << ident << "(";
            Symtab *cnt = symtab->NewSymTab();
            cnt_func_symtab = cnt;
            func_f_params->dfs(ans, aststack, cnt);
            ans += ")";
            cout << ")";
            // func_type->dfs(ans, aststack, symtab);
            if (func_type == "int")
            {
                ans += ": i32";
                cout << ": i32";
                is_void_func = false;
            }
            else
            {
                is_void_func = true;
            }
            symtab->InsertFunc(ident, is_void_func);
            ans += "{\n";
            string namename = "\%e" + to_string(name_count++);
            ans += namename + ":\n";
            cout << "{\n"
                 << namename << ":\n";
            if (is_void_func == false)
            {
                // cout<<333<<endl;
                ans += "\t%ret = alloc i32\n";
                string endname = "\%end" + to_string(mystart_count);
                string mystartname = "\%mystart" + to_string(mystart_count++);
                ans += "\tjump " + mystartname + "\n";
                ans += mystartname + ":\n";
                cout << "\t%ret = alloc i32\n\tjump " + mystartname + "\n" + mystartname + ":\n";
                block->dfs(ans, aststack, symtab);
                //需要考虑前面有没有分支跳转 buggy
                ans += "\tjump " + endname + "\n";
                ans += endname + ":\n";
                string last_sym = "\%" + to_string(tpname_count++);
                ans += "\t" + last_sym + " = load %ret\n";
                ans += "\tret " + last_sym + "\n";
                ans += "}\n";
                cout << "\tjump " << endname << "\n"
                     << endname << ":\n\t" << last_sym << " = load %ret\n\tret " << last_sym << "\n}\n";
            }
            else
            {
                // cout<<444<<endl;
                string endname = "\%end" + to_string(mystart_count);
                string mystartname = "\%mystart" + to_string(mystart_count++);
                ans += "\tjump " + mystartname + "\n";
                ans += mystartname + ":\n";
                cout << "\tjump " << mystartname << "\n"
                     << mystartname << ":\n";
                block->dfs(ans, aststack, symtab);
                //需要考虑前面有没有分支跳转 buggy
                ans += "\tjump " + endname + "\n";
                ans += endname + ":\n";
                ans += "\tret\n";
                ans += "}\n";
                cout << "\tjump " << endname << "\n"
                     << endname << ":\n\tret\n}\n";
            }
        }
    }
};

class FuncFParamsAST : public BaseAST
{
public:
    unique_ptr<BaseAST> func_f_params;
    unique_ptr<BaseAST> func_f_param;
    void Dump(string &ast) const override
    {
        cout << 1 << endl;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (func_f_params != nullptr)
        {
            func_f_params->dfs(ans, aststack, symtab);
            ans += ", ";
            cout << ", ";
            func_f_param->dfs(ans, aststack, symtab);
        }
        else
        {
            func_f_param->dfs(ans, aststack, symtab);
        }
    }
};

class FuncFParamAST : public BaseAST
{
public:
    string ident;
    unique_ptr<BaseAST> const_arr;
    void Dump(string &ast) const override
    {
        cout << 1 << endl;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_arr == nullptr)
        {
            ans += "@" + ident + ": i32";
            cout << "@" << ident << ": i32";
            //接下来要在当前函数符号表里插入参数，并且函数开头需要给它们分配空间
            //使用独特的符号表，把形参转换为需要的参数，存在这个函数顶层符号表里，然后形参就没用了
            symtab->InsertVar(ident, "\%" + ident);
        }
        else //数组作为参数，需要考虑的问题比较多
        {
            ans += "@" + ident + ": ";
            cout << "@" << ident << ": ";
            is_arr = false;
            arr_param.clear();
            const_arr->dfs(ans, aststack, symtab);
            int len = arr_param.size();
            // symtab->Print();
            string allocstring = "";
            if (len != 0)
            {
                genPtrAlloc(allocstring, arr_param, len, 1);
                ans += "*" + allocstring;
                cout << "*" << allocstring;
            }
            else
            {
                ans += "*i32";
                cout << "*i32";
                allocstring = "i32";
            }
            symtab->InsertPtr(ident, "\%" + ident, len + 1, arr_param, allocstring);
        }
    }
    void genPtrAlloc(string &tpans, vector<int> &params, int dime, int layer) const
    {
        if (dime == layer)
        {
            tpans += "[i32, " + to_string(params[layer - 1]) + "]";
            return;
        }
        genPtrAlloc(tpans, params, dime, layer + 1);
        tpans = "[" + tpans + ", " + to_string(params[layer - 1]) + "]";
        return;
    }
};

class FuncRParamsAST : public BaseAST
{
public:
    unique_ptr<BaseAST> func_r_params;
    unique_ptr<BaseAST> exp;
    void Dump(string &ast) const override
    {
        cout << 1 << endl;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (func_r_params != nullptr)
        {
            // cout<<"param1"<<endl;
            func_r_params->dfs(ans, aststack, symtab);
            exp->dfs(ans, aststack, symtab);
        }
        else
        {
            // cout<<"param2"<<endl;
            exp->dfs(ans, aststack, symtab);
        }
    }
};

class BlockAST : public BaseAST
{
public:
    unique_ptr<BaseAST> block_item_arr;

    void Dump(string &ast) const override
    {
        cout << "BlockAST { ";
        ast += "BlockAST { ";
        block_item_arr->Dump(ast);
        cout << " }";
        ast += " }";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        Symtab *cnt;
        //有参数的函数的情况，处理符号表以及打印里面分配的参数
        if (use_cnt_tab == true)
        {
            cnt = cnt_func_symtab;
            use_cnt_tab = false;
            int param = cnt->table_size;
            for (int i = 0; i < param; i++)
            {
                auto entry = cnt->table[i];
                if (entry->ty == 1)
                {
                    VarEntry *ttp = (VarEntry *)entry;
                    ttp->is_param = true;
                    ans += "\t" + ttp->var_sym + " = alloc i32\n";
                    cout << "\t" << ttp->var_sym << " = alloc i32\n";
                    ans += "\tstore @" + ttp->sym_name + ", " + ttp->var_sym + "\n";
                    cout << "\tstore @" << ttp->sym_name << ", " << ttp->var_sym << endl;
                }
                else if (entry->ty == 4)
                {
                    PtrEntry *ttp = (PtrEntry *)entry;
                    ttp->is_param = true;
                    ans += "\t" + ttp->ptr_sym + " = alloc *" + ttp->alloc_string + "\n";
                    cout << "\t" << ttp->ptr_sym << " = alloc *" << ttp->alloc_string << endl;
                    ans += "\tstore @" + ttp->sym_name + ", " + ttp->ptr_sym + "\n";
                    cout << "\tstore @" << ttp->sym_name << ", " << ttp->ptr_sym << endl;
                }
            }
        }
        else
        {
            cnt = symtab->NewSymTab();
        }
        // cout<<"BlockAST"<<endl;
        block_item_arr->dfs(ans, aststack, cnt);
        cnt->DeleteSymTab();
    }
};

class BlockItemArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> block_item_arr;
    unique_ptr<BaseAST> block_item;

    void Dump(string &ast) const override
    {
        //我不想实现了，不调用就好了
        cout << 1;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (block_item_arr != nullptr)
        {
            block_item_arr->dfs(ans, aststack, symtab);
            block_item->dfs(ans, aststack, symtab);
        }
        else if (block_item != nullptr)
        {
            block_item->dfs(ans, aststack, symtab);
        }
    }
};

class BlockItemAST : public BaseAST
{
public:
    unique_ptr<BaseAST> stmt;
    unique_ptr<BaseAST> decl;

    void Dump(string &ast) const override
    {
        cout << 2;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (stmt != nullptr)
        {
            stmt->dfs(ans, aststack, symtab);
        }
        else
        {
            decl->dfs(ans, aststack, symtab);
        }
        last_ret = false;
        genAST(ans, aststack, symtab);
        if (last_ret == true)
        {
            string bbname = "\%e" + to_string(name_count++);
            ans += bbname + ":\n";
            cout << bbname << ":\n";
        }

        while (!aststack.empty())
            aststack.pop();
    }
};

class DeclAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_decl;
    unique_ptr<BaseAST> var_decl;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_decl != nullptr)
        {
            const_decl->dfs(ans, aststack, symtab);
        }
        else if (var_decl != nullptr)
        {
            var_decl->dfs(ans, aststack, symtab);
        }
    }
};

class VarDeclAST : public BaseAST
{
public:
    unique_ptr<BaseAST> var_def_arr;

    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        var_def_arr->dfs(ans, aststack, symtab);
    }
};

class VarDefArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> var_def_arr;
    unique_ptr<BaseAST> var_def;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (var_def_arr != nullptr)
        {
            var_def_arr->dfs(ans, aststack, symtab);
            var_def->dfs(ans, aststack, symtab);
        }
        else
        {
            var_def->dfs(ans, aststack, symtab);
        }
    }
};

class VarDefAST : public BaseAST
{
public:
    unique_ptr<BaseAST> init_val;
    unique_ptr<BaseAST> const_arr;
    string ident;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void fill_dfs(string &ans, Symtab *symtab, int dim, int layer, string parent) const
    {
        if (dim == layer)
            return;
        if (dim != layer && arr_is_const)
        {
            cout << "{";
            ans += "{";
        }
        int scala = arr_param[layer];
        for (int i = 0; i < scala; i++)
        {
            if (arr_is_const)
            {
                if (layer == dim - 1)
                {
                    ans += arr_init_table[arr_total++];
                    cout << arr_init_table[arr_total - 1];
                    if (i != scala - 1)
                    {
                        ans += ", ";
                        cout << ", ";
                    }
                }
                if (i != scala - 1)
                {
                    fill_dfs(ans, symtab, dim, layer + 1, "");
                }
                else
                {
                    fill_dfs(ans, symtab, dim, layer + 1, "1");
                }
            }
            else
            {
                string tpname = "\%p" + to_string(tpname_count++);
                ans += "\t" + tpname + " = getelemptr " + parent + ", " + to_string(i) + "\n";
                cout << "\t" << tpname << " = getelemptr " << parent << ", " << i << endl;
                if (layer == dim - 1)
                {
                    string tar = arr_init_table[arr_total++];
                    auto tp2 = symtab->QueryUp(tar);
                    if (tp2 == nullptr)
                    {
                        ans += "\tstore " + tar + ", " + tpname + "\n";
                        cout << "\tstore " << tar << ", " << tpname << endl;
                    }
                    else
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *tsp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + tsp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << tsp->var_sym << endl;
                        ans += "\tstore " + ttpname + ", " + tpname + "\n";
                        cout << "\tstore " << ttpname << ", " << tpname << endl;
                    }
                }
                fill_dfs(ans, symtab, dim, layer + 1, tpname);
            }
        }
        if (dim != layer && arr_is_const)
        {
            cout << "}";
            ans += "}";
            if (parent == "")
            {
                ans += ", ";
                cout << ", ";
            }
        }
    }
    void FillInitArr(string &ans, Symtab *symtab) const
    {
        string tpname;
        auto tp = symtab->QueryUp(ident);
        ArrEntry *ttp = (ArrEntry *)tp;
        string arrsym = ttp->arr_sym;
        arr_total = 0;
        int dim = arr_param.size();
        fill_dfs(ans, symtab, dim, 0, arrsym);
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        is_arr = false;
        arr_param.clear();
        while(!aststack.empty()) aststack.pop();
        const_arr->dfs(ans, aststack, symtab);
         //cout<<arr_param.size()<<endl;
         //if(arr_param.size()) cout<<arr_param[0]<<endl;
        // cout<<ident<<", "<<is_arr<<endl;
        if (is_arr == false) //普通变量
        {
            int layer = symtab->layer;
            if (layer != 0) //局部
            {
                if (init_val != nullptr)
                {
                    aststack.push(ident);
                    aststack.push("=");
                    symtab->InsertVar(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
                    init_val->dfs(ans, aststack, symtab);
                    ans += "\t@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32\n";
                    cout << "\t@" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc i32" << endl;
                    last_ret = false;
                    genAST(ans, aststack, symtab);
                }
                else
                {
                    symtab->InsertVar(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
                    ans += "\t@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32\n";
                    cout << "\t@" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc i32" << endl;
                }
            }
            else //全局
            {
                if (init_val != nullptr)
                {
                    symtab->InsertVar(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
                    //求值
                    stack<string> fuzhu;
                    while (!aststack.empty())
                    {
                        fuzhu.push(aststack.top());
                        aststack.pop();
                    }
                    init_val->dfs(ans, aststack, symtab);
                    int cal_val = cal(aststack, symtab);
                    while (!fuzhu.empty())
                    {
                        aststack.push(fuzhu.top());
                        fuzhu.pop();
                    }
                    ans += "global @" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32, " + to_string(cal_val) + "\n";
                    cout << "global @" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc i32, " << cal_val << endl;
                }
                else
                {
                    symtab->InsertVar(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
                    ans += "global @" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32, zeroinit\n";
                    cout << "global @" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc i32, zeroinit" << endl;
                }
            }
        }
        else //变量数组
        {
            int dimension = arr_param.size();
            arr_layer = 0;
            int layer = symtab->layer;
            if (layer != 0) //局部
            {
                if (init_val != nullptr)
                {
                    arr_init_table.clear();
                    symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), false, dimension, arr_param);
                    ans += "\t@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                    cout << "\t@" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                    string allocstring = "";
                    cout<<endl;
                    for(int j = 0; j < arr_param.size(); j++) cout<<arr_param[j]<<" ";
                    cout<<dimension<<endl;
                    genArrAlloc(allocstring, arr_param, dimension, 1);
                    cout << allocstring << endl;
                    ans += allocstring + "\n";
                    str_arr_init = "";
                    init_val->dfs(ans, aststack, symtab);
                    // cout<<str_arr_init<<endl;
                    // for(int i = 0; i < arr_init_table.size();i++) cout<<arr_init_table[i]<<" ";
                    // cout<<endl;
                    FillInitArr(ans, symtab);
                }
                else
                {
                    symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), false, dimension, arr_param);
                    ans += "\t@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                    cout << "\t@" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                    string allocstring = "";
                    genArrAlloc(allocstring, arr_param, dimension, 1);
                    cout << allocstring << endl;
                    ans += allocstring + "\n";
                }
            }
            else //全局
            {
                if (init_val != nullptr)
                {
                    arr_is_const = true;
                    arr_init_table.clear();
                    symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), false, dimension, arr_param);
                    ans += "global @" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                    cout << "global @" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                    string allocstring = "";
                    genArrAlloc(allocstring, arr_param, dimension, 1);
                    cout << allocstring << ", ";
                    ans += allocstring + ", ";
                    str_arr_init = "";
                    init_val->dfs(ans, aststack, symtab);
                    // for(int i = 0; i < arr_init_table.size();i++) cout<<arr_init_table[i]<<" ";
                    // cout<<endl;
                    FillInitArr(ans, symtab);
                    ans += "\n";
                    cout << "\n";
                    arr_is_const = false;
                }
                else
                {
                    symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), false, dimension, arr_param);
                    ans += "global @" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                    cout << "global @" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                    string allocstring = "";
                    genArrAlloc(allocstring, arr_param, dimension, 1);
                    cout << allocstring << ", zeroinit" << endl;
                    ans += allocstring + ", zeroinit\n";
                }
            }
        }
        if (is_arr)
            is_arr = false;
    }
    void genArrAlloc(string &tpans, vector<int> &params, int dime, int layer) const
    {
        if (dime == layer)
        {
            tpans += "[i32, " + to_string(params[layer - 1]) + "]";
            return;
        }
        genArrAlloc(tpans, params, dime, layer + 1);
        tpans = "[" + tpans + ", " + to_string(params[layer - 1]) + "]";
        return;
    }
};

class InitValAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> init_val_arr;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (exp != nullptr)
        {
            exp->dfs(ans, aststack, symtab);
            if (is_arr && !arr_is_const)
            {
                genAST(ans, aststack, symtab);
                string tp = aststack.top();
                aststack.pop();
                str_arr_init += tp;
                arr_init_table.push_back(tp);
            }
            else if (is_arr && arr_is_const) // global var arr
            {
                int a = cal(aststack, symtab);
                str_arr_init += to_string(a);
                arr_init_table.push_back(to_string(a));
            }
        }
        else if (init_val_arr != nullptr)
        {
            str_arr_init += "{";
            arr_layer++;
            int tptot = arr_init_table.size();
            int pre_len = tptot;
            int len = arr_param.size();
            int maxn = 1;
            for (int i = len - 1; i >= arr_layer - 1; --i)
            {
                if (tptot % arr_param[i] == 0)
                {
                    maxn *= arr_param[i];
                    tptot /= arr_param[i];
                }
                else
                    break;
            }
            init_val_arr->dfs(ans, aststack, symtab);
            int later_len = arr_init_table.size();
            for (int i = later_len; i < pre_len + maxn; ++i)
            {
                arr_init_table.push_back("0");
            }
            arr_layer--;
            str_arr_init += "}";
        }
        else //初始化为全0
        {
            arr_layer++;
            int tptot = arr_init_table.size();
            int len = arr_param.size();
            int maxn = 1;
            for (int i = len - 1; i >= arr_layer - 1; --i)
            {
                if (tptot % arr_param[i] == 0)
                {
                    maxn *= arr_param[i];
                    tptot /= arr_param[i];
                }
                else
                    break;
            }
            for (int i = 0; i < maxn; ++i)
            {
                arr_init_table.push_back("0");
            }
            str_arr_init += "{}";
            arr_layer--;
        }
    }
};

class InitValArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> init_val_arr;
    unique_ptr<BaseAST> init_val;

    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (init_val_arr != nullptr)
        {
            init_val_arr->dfs(ans, aststack, symtab);
            str_arr_init += ",";
            init_val->dfs(ans, aststack, symtab);
        }
        else
        {
            init_val->dfs(ans, aststack, symtab);
        }
    }
};

class ConstDeclAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_def_arr;

    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // cout<<"Const Decl"<<endl;
        const_def_arr->dfs(ans, aststack, symtab);
    }
};

class ConstDefArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_def_arr;
    unique_ptr<BaseAST> const_def;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_def_arr != nullptr)
        {
            const_def_arr->dfs(ans, aststack, symtab);
            const_def->dfs(ans, aststack, symtab);
        }
        else
        {
            const_def->dfs(ans, aststack, symtab);
        }
    }
};

class ConstDefAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_init_val;
    unique_ptr<BaseAST> const_arr;
    string ident;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void fill_dfs(string &ans, Symtab *symtab, int dim, int layer, string parent) const
    {
        if (dim == layer)
            return;
        if (dim != layer && arr_is_const)
        {
            cout << "{";
            ans += "{";
        }
        int scala = arr_param[layer];
        for (int i = 0; i < scala; i++)
        {
            if (arr_is_const)
            {
                if (layer == dim - 1)
                {
                    ans += arr_init_table[arr_total++];
                    cout << arr_init_table[arr_total - 1];
                    if (i != scala - 1)
                    {
                        ans += ", ";
                        cout << ", ";
                    }
                }
                if (i != scala - 1)
                {
                    fill_dfs(ans, symtab, dim, layer + 1, "");
                }
                else
                {
                    fill_dfs(ans, symtab, dim, layer + 1, "1");
                }
            }
            else
            {
                string tpname = "\%p" + to_string(tpname_count++);
                ans += "\t" + tpname + " = getelemptr " + parent + ", " + to_string(i) + "\n";
                cout << "\t" << tpname << " = getelemptr " << parent << ", " << i << endl;
                if (layer == dim - 1)
                {
                    string tar = arr_init_table[arr_total++];
                    auto tp2 = symtab->QueryUp(tar);
                    if (tp2 == nullptr)
                    {
                        ans += "\tstore " + tar + ", " + tpname + "\n";
                        cout << "\tstore " << tar << ", " << tpname << endl;
                    }
                    else
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *tsp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + tsp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << tsp->var_sym << endl;
                        ans += "\tstore " + ttpname + ", " + tpname + "\n";
                        cout << "\tstore " << ttpname << ", " << tpname << endl;
                    }
                }
                fill_dfs(ans, symtab, dim, layer + 1, tpname);
            }
        }
        if (dim != layer && arr_is_const)
        {
            cout << "}";
            ans += "}";
            if (parent == "")
            {
                ans += ", ";
                cout << ", ";
            }
        }
    }
    void FillInitArr(string &ans, Symtab *symtab) const
    {
        string tpname;
        auto tp = symtab->QueryUp(ident);
        ArrEntry *ttp = (ArrEntry *)tp;
        string arrsym = ttp->arr_sym;
        arr_total = 0;
        int dim = arr_param.size();
        fill_dfs(ans, symtab, dim, 0, arrsym);
    }
    void genArrAlloc(string &tpans, vector<int> &params, int dime, int layer) const
    {
        if (dime == layer)
        {
            tpans += "[i32, " + to_string(params[layer - 1]) + "]";
            return;
        }
        genArrAlloc(tpans, params, dime, layer + 1);
        tpans = "[" + tpans + ", " + to_string(params[layer - 1]) + "]";
        return;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        is_arr = false;
        //添加符号表的操作，需要在constdefast里面进行符号表操作，并写出编译求值函数
        // cout<<"Const Def"<<endl;
        arr_param.clear();
        while(!aststack.empty()) aststack.pop();
        const_arr->dfs(ans, aststack, symtab);
        if (is_arr == false) //一般常量
        {
            exp_is_const = true;
            cal_val = 0;
            while (!const_val_stack.empty())
                const_val_stack.pop();
            const_init_val->dfs(ans, const_val_stack, symtab);
            int const_val = cal(const_val_stack, symtab);
            symtab->InsertConst(ident, const_val);
            // symtab->Print();
            exp_is_const = false;
        }
        else //常量数组，需要分配空间
        {
            int dimension = arr_param.size();
            arr_layer = 0;
            int layer = symtab->layer;
            if (layer != 0) //局部
            {
                arr_init_table.clear();
                symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), true, dimension, arr_param);
                ans += "\t@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                cout << "\t@" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                string allocstring = "";
                genArrAlloc(allocstring, arr_param, dimension, 1);
                cout << allocstring << endl;
                ans += allocstring + "\n";
                str_arr_init = "";
                const_init_val->dfs(ans, aststack, symtab);
                cout << str_arr_init << endl;
                for (int i = 0; i < arr_init_table.size(); i++)
                    cout << arr_init_table[i] << " ";
                cout << endl;
                FillInitArr(ans, symtab);
            }
            else //全局
            {
                arr_is_const = true;
                arr_init_table.clear();
                symtab->InsertArr(ident, "@" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no), true, dimension, arr_param);
                ans += "global @" + ident + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc ";
                cout << "global @" << ident << "_" << symtab->layer << "_" << symtab->child_no << " = alloc ";
                string allocstring = "";
                genArrAlloc(allocstring, arr_param, dimension, 1);
                cout << allocstring << ", ";
                ans += allocstring + ", ";
                str_arr_init = "";
                const_init_val->dfs(ans, aststack, symtab);
                // for(int i = 0; i < arr_init_table.size();i++) cout<<arr_init_table[i]<<" ";
                // cout<<endl;
                FillInitArr(ans, symtab);
                ans += "\n";
                cout << "\n";
                arr_is_const = false;
            }
        }
        if (is_arr)
            is_arr = false;
    }
};

class ConstArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_arr;
    unique_ptr<BaseAST> const_exp;
    void Dump(string &ast) const override
    {
        cout << 1;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_arr != nullptr)
        {
            is_arr = true;
            const_arr->dfs(ans, aststack, symtab);
            const_exp->dfs(ans, aststack, symtab);
            int array_size = cal(aststack, symtab);
            //cout<<"size: "<<array_size<<endl;
            arr_param.push_back(array_size);
        }
    }
};

class ConstInitValAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_exp;
    unique_ptr<BaseAST> const_init_val_arr;

    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_exp != nullptr)
        {
            const_exp->dfs(ans, aststack, symtab);
            if (is_arr && !arr_is_const)
            {
                genAST(ans, aststack, symtab);
                string tp = aststack.top();
                aststack.pop();
                str_arr_init += tp;
                arr_init_table.push_back(tp);
            }
            else if (is_arr && arr_is_const) // global var arr
            {
                int a = cal(aststack, symtab);
                str_arr_init += to_string(a);
                arr_init_table.push_back(to_string(a));
            }
        }
        else if (const_init_val_arr != nullptr)
        {
            str_arr_init += "{";
            arr_layer++;
            int tptot = arr_init_table.size();
            int pre_len = tptot;
            int len = arr_param.size();
            int maxn = 1;
            for (int i = len - 1; i >= arr_layer - 1; --i)
            {
                if (tptot % arr_param[i] == 0)
                {
                    maxn *= arr_param[i];
                    tptot /= arr_param[i];
                }
                else
                    break;
            }
            const_init_val_arr->dfs(ans, aststack, symtab);
            int later_len = arr_init_table.size();
            for (int i = later_len; i < pre_len + maxn; ++i)
            {
                arr_init_table.push_back("0");
            }
            arr_layer--;
            str_arr_init += "}";
        }
        else //初始化为全0
        {
            arr_layer++;
            int tptot = arr_init_table.size();
            int len = arr_param.size();
            int maxn = 1;
            for (int i = len - 1; i >= arr_layer - 1; --i)
            {
                if (tptot % arr_param[i] == 0)
                {
                    maxn *= arr_param[i];
                    tptot /= arr_param[i];
                }
                else
                    break;
            }
            for (int i = 0; i < maxn; ++i)
            {
                arr_init_table.push_back("0");
            }
            str_arr_init += "{}";
            arr_layer--;
        }
    }
};

class ConstInitValArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> const_init_val_arr;
    unique_ptr<BaseAST> const_init_val;

    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (const_init_val_arr != nullptr)
        {
            const_init_val_arr->dfs(ans, aststack, symtab);
            const_init_val->dfs(ans, aststack, symtab);
        }
        else
        {
            const_init_val->dfs(ans, aststack, symtab);
        }
    }
};

class ConstExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp;
    void Dump(string &ast) const override
    {
        cout << 3;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // cout<<"Const Exp"<<endl;
        exp->dfs(ans, aststack, symtab);
        // cout<<"Size2:"<<aststack.size()<<endl;
    }
};

class StmtAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    unique_ptr<BaseAST> block;
    unique_ptr<BaseAST> condition;
    unique_ptr<BaseAST> stmt1;
    unique_ptr<BaseAST> stmt2;
    string stmt; // return,if,ifelse,while

    void Dump(string &ast) const override
    {
        //废弃的函数，懒得改了
        cout << 1;
    }
    // buggy
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (lval != nullptr)
        {
            lval->dfs(ans, aststack, symtab);
            aststack.push("=");
            exp->dfs(ans, aststack, symtab);
        }
        else if (block != nullptr)
        {
            block->dfs(ans, aststack, symtab);
        }
        else if (exp != nullptr && stmt == "return")
        {
            aststack.push("ret");
            exp->dfs(ans, aststack, symtab);
        }
        else if (exp != nullptr && stmt == "while")
        {
            string entry_name = "\%while_entry_" + to_string(while_count);
            string body_name = "\%while_body_" + to_string(while_count);
            string end_name = "\%while_end_" + to_string(while_count++);
            ans += "\tjump " + entry_name + "\n";
            ans += entry_name + ":\n";
            cout << "\tjump " << entry_name << "\n"
                 << entry_name << ":\n";
            exp->dfs(ans, aststack, symtab);
            genAST(ans, aststack, symtab);
            string r_name = aststack.top();
            aststack.pop();
            if ((r_name[0] == '%' && r_name[1] != 'p' && r_name[1] != 'q') || isdigit(r_name[0]))
            {
                ans += "\tbr " + r_name + ", " + body_name + ", " + end_name + "\n";
                // cout<<"\tbr "<<r_name<<", "<<body_name<<", "<<end_name<<endl;
            }
            else
            {
                symtab->Print();
                auto tp = symtab->QueryUp(r_name);
                if (tp == nullptr)
                    cout << r_name << ", is nullptr\n";
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans += "\t%" + tpname + " = load " + varsym + "\n";
                ans += "\tbr %" + tpname + ", " + body_name + ", " + end_name + "\n";
                // cout<<"\t%"<<tpname<<" = load "<<varsym<<endl;
                // cou t<<"\tbr %"<<tpname<<", "<<body_name<<", "<<end_name<<endl;
            }
            ans += body_name + ":\n";
            cout << body_name << ":" << endl;
            auto ntable = symtab->NewSymTab();
            bool outer_flag = false;
            if (outer_while == false)
            {
                outer_flag = true;
                outer_while = true;
                current_while = while_count - 1;
            }
            else
            {
                current_while++;
            }
            stmt1->dfs(ans, aststack, ntable);
            is_ret_inside = false;
            genAST(ans, aststack, symtab);
            ntable->DeleteSymTab();
            current_while--;
            while (!aststack.empty())
                aststack.pop();
            if (!is_ret_inside)
                ans += "\tjump " + entry_name + "\n";
            is_ret_inside = false;
            ans += end_name + ":\n";
            if (outer_flag)
            {
                outer_while = false;
            }
            // cout<<end_name<<endl;
        }
        else if (exp != nullptr)
        {
            exp->dfs(ans, aststack, symtab);
        }
        else if (stmt == "break")
        {
            string end_name = "\%while_end_" + to_string(current_while);
            ans += "\tjump " + end_name + "\n";
            ans += "\%while_not_" + to_string(while_not_count++) + ":\n";
        }
        else if (stmt == "continue")
        {
            string entry_name = "\%while_entry_" + to_string(current_while);
            ans += "\tjump " + entry_name + "\n";
            ans += "\%while_not_" + to_string(while_not_count++) + ":\n";
        }
        else if (stmt == "return")
        {
            aststack.push("retvoid");
        }
        else if (stmt == "if") // if (cond) stmt1
        {
            is_ret_inside = false;
            bool tpflag = false;
            if (inside_if_else == true)
                tpflag = true;
            else
            {
                inside_if_else = true;
                tpflag = false;
            }
            condition->dfs(ans, aststack, symtab);
            last_ret = false;
            genAST(ans, aststack, symtab);
            string r_name = aststack.top();
            aststack.pop();
            string ifname = "\%if_" + to_string(if_count++);
            string thenname = "\%then_" + to_string(then_count++);
            if ((r_name[0] == '%' && r_name[1] != 'p' && r_name[1] != 'q') || isdigit(r_name[0]))
            {
                ans += "\tbr " + r_name + ", " + ifname + ", " + thenname + "\n";
            }
            else
            {
                auto tp = symtab->QueryUp(r_name);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans += "\t%" + tpname + " = load " + varsym + "\n";
                ans += "\tbr %" + tpname + ", " + ifname + ", " + thenname + "\n";
            }
            ans += ifname + ":\n";
            auto ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt1->dfs(ans, aststack, ntable);
            // cout<<aststack.size()<<endl;
            last_ret = false;
            genAST(ans, aststack, ntable);
            ret_in_branch = false;
            while (!aststack.empty())
                aststack.pop();
            ntable->DeleteSymTab();
            if (!is_ret_inside)
                ans += "\tjump " + thenname + "\n";
            is_ret_inside = false;
            ans += thenname + ":\n";
            if (!tpflag)
                inside_if_else = false;
        }
        else if (stmt == "ifelse") // if (cond) stmt1 else stmt2
        {
            is_ret_inside = false;
            bool tpflag = false;
            if (inside_if_else == true)
                tpflag = true;
            else
            {
                inside_if_else = true;
                tpflag = false;
            }
            condition->dfs(ans, aststack, symtab);
            last_ret = false;
            genAST(ans, aststack, symtab);
            string r_name = aststack.top();
            aststack.pop();
            string ifname = "\%if_" + to_string(if_count++);
            string thenname = "\%then_" + to_string(then_count++);
            string elsename = "\%else_" + to_string(else_count++);
            if ((r_name[0] == '%' && r_name[1] != 'p' && r_name[1] != 'q') || isdigit(r_name[0]))
            {
                ans += "\tbr " + r_name + ", " + ifname + ", " + elsename + "\n";
            }
            else
            {
                auto tp = symtab->QueryUp(r_name);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans += "\t%" + tpname + " = load " + varsym + "\n";
                ans += "\tbr %" + tpname + ", " + ifname + ", " + elsename + "\n";
            }
            ans += ifname + ":\n";
            auto ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt1->dfs(ans, aststack, ntable);
            last_ret = false;
            genAST(ans, aststack, ntable);
            ret_in_branch = false;
            while (!aststack.empty())
                aststack.pop();
            ntable->DeleteSymTab();
            if (!is_ret_inside)
                ans += "\tjump " + thenname + "\n";
            ans += elsename + ":\n";
            is_ret_inside = false;
            ntable = symtab->NewSymTab();
            ret_in_branch = true;
            stmt2->dfs(ans, aststack, ntable);
            last_ret = false;
            genAST(ans, aststack, ntable);
            ret_in_branch = false;
            while (!aststack.empty())
                aststack.pop();
            ntable->DeleteSymTab();
            if (!is_ret_inside)
            {
                ans += "\tjump " + thenname + "\n";
            }
            ans += thenname + ":\n";
            is_ret_inside = false;
            if (!tpflag)
                inside_if_else = false;
        }
        else
        {
            cout << ""; // make compiler happy
        }
    }
};

class ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> lor_exp;
    void Dump(string &ast) const override
    {
        cout << "ExpAST { ";
        ast += "ExpAST { ";
        lor_exp->Dump(ast);
        cout << " }";
        ast += " }";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (is_cal_param)
        {
            stack<string> fuzhu;
            while (!aststack.empty())
            {
                fuzhu.push(aststack.top());
                aststack.pop();
            }
            lor_exp->dfs(ans, aststack, symtab);
            genAST(ans, aststack, symtab);
            string top_name = aststack.top();
            aststack.pop();
            // cout << top_name << ": " << param_is_arr << endl;

            if (top_name[0] == '%' && top_name[1] == 'p')
            {
                string tpname = "%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = getelemptr " + top_name + ", 0\n";
                cout << "\t" << tpname << " = getelemptr " << top_name << ", 0\n";
                top_name = tpname;
            }
            else if (top_name[0] == '%' && top_name[1] == 'q')
            {
                string tpname = "%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = load " + top_name + "\n";
                cout << "\t" << tpname << " = load " << top_name << endl;
                top_name = tpname;
            }

            auto tp = symtab->QueryUp(top_name);
            if (tp == nullptr)
            {
                if (param_is_arr)
                    aststack.push(top_name);
                else
                    tp_params_sym.push(top_name);
            }
            else
            {
                string tpname = "%" + to_string(tpname_count++);
                VarEntry *ttp = (VarEntry *)tp;
                ans += "\t" + tpname + " = load " + ttp->var_sym + "\n";
                cout << "\t" << tpname << " = load " << ttp->var_sym << endl;
                if (param_is_arr)
                    aststack.push(tpname);
                else
                    tp_params_sym.push(tpname);
            }
            while (!fuzhu.empty())
            {
                aststack.push(fuzhu.top());
                fuzhu.pop();
            }
        }
        else
        {
            lor_exp->dfs(ans, aststack, symtab);
            // cout<<"print: ";
            // PrintStack(aststack);
        }
    }
};

class UnaryExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> primary_exp;
    unique_ptr<BaseAST> unary_op;
    unique_ptr<BaseAST> unary_exp;
    unique_ptr<BaseAST> func_r_params;
    string ident;
    void Dump(string &ast) const override
    {
        cout << 1;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (primary_exp != nullptr)
        {
            primary_exp->dfs(ans, aststack, symtab);
        }
        else if (unary_op != nullptr && unary_exp != nullptr)
        {
            unary_op->dfs(ans, aststack, symtab);
            unary_exp->dfs(ans, aststack, symtab);
        }
        //有参数的函数调用，先遍历参数再call
        else if (func_r_params != nullptr)
        {
            or_and_layer++;
            bool outer_param = false;
            if (is_cal_param == false)
            {
                is_cal_param = true;
                outer_param = true;
                // cout<<ident<<endl;
            }
            auto tp = symtab->QueryUp(ident);
            FuncEntry *ttp = (FuncEntry *)tp;
            queue<string> cunchu;
            while (!tp_params_sym.empty())
            {
                cunchu.push(tp_params_sym.front());
                tp_params_sym.pop();
            }
            if (ttp->void_func) //无返回值，只需要call就可以了
            {
                func_r_params->dfs(ans, aststack, symtab);
                cout << "\tcall @" << ident << "(";
                ans += "\tcall @" + ident + "(";
                ans += tp_params_sym.front();
                cout << tp_params_sym.front();
                tp_params_sym.pop();
                while (!tp_params_sym.empty())
                {
                    ans += ", " + tp_params_sym.front();
                    cout << ", " << tp_params_sym.front();
                    tp_params_sym.pop();
                }
                ans += ")\n";
                cout << ")" << endl;
            }
            else
            {
                func_r_params->dfs(ans, aststack, symtab);
                // cout<<"tpsize: "<<tp_params_sym.size()<<endl;
                string tpname = "\%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = ";
                cout << "\t" << tpname << " = ";
                cout << "call @" << ident << "(";
                ans += "call @" + ident + "(";
                ans += tp_params_sym.front();
                cout << tp_params_sym.front();
                tp_params_sym.pop();
                while (!tp_params_sym.empty())
                {
                    ans += ", " + tp_params_sym.front();
                    cout << ", " << tp_params_sym.front();
                    tp_params_sym.pop();
                }
                ans += ")\n";
                cout << ")" << endl;
                aststack.push(tpname);
                // cout<<"123: "<<tpname<<endl;
            }
            while (!cunchu.empty())
            {
                tp_params_sym.push(cunchu.front());
                cunchu.pop();
            }
            if (outer_param)
            {
                is_cal_param = false;
            }
            or_and_layer--;
        }
        //无参数的函数调用，直接call
        else
        {
            auto tp = symtab->QueryUp(ident);
            FuncEntry *ttp = (FuncEntry *)tp;
            if (ttp->void_func)
            {
                cout << "\tcall @" << ident << "()" << endl;
                ans += "\tcall @" + ident + "()\n";
            }
            else
            {
                string tpname = "\%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = ";
                cout << "\t" << tpname << " = ";
                cout << "call @" << ident << "()\n";
                ans += "call @" + ident + "()\n";
                aststack.push(tpname);
            }
        }
    }
};

class LValAST : public BaseAST
{
public:
    string ident;
    unique_ptr<BaseAST> exp_arr;

    void Dump(string &ast) const override
    {
        cout << ident << " ";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        auto ptp = symtab->QueryUp(ident);
        if (ptp != nullptr)
        {
            if (ptp->ty == 0)
            {
                ConstEntry *pptp = (ConstEntry *)ptp;
                aststack.push(to_string(pptp->const_val));
                return;
            }
        }
        is_arr = false;
        stack<string> fuzhu;
        while (!aststack.empty())
        {
            fuzhu.push(aststack.top());
            aststack.pop();
        }
        arr_param_layer++;
        exp_arr->dfs(ans, aststack, symtab);
        arr_param_layer--;
        while (!fuzhu.empty())
        {
            aststack.push(fuzhu.top());
            fuzhu.pop();
        }
        cout<<123<<":";
         cout<<ident<<", "<<is_arr<<endl;
        if (!is_arr) //普通lval
        {
            auto tp = symtab->QueryUp(ident);
            if (tp == nullptr)
            {
                cerr << "No declaration before" << endl;
                exit(1);
            }
            int sym_type = tp->ty;
            // cout<<ident<<", "<<sym_type<<endl;
            if (sym_type == 0)
            {
                ConstEntry *ttp = (ConstEntry *)tp;
                int val = ttp->const_val;
                aststack.push(to_string(val));
            }

            else if (sym_type == 3) // array
            {
                ArrEntry *ttp = (ArrEntry *)tp;
                string tpname = "%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = getelemptr " + ttp->arr_sym + ", 0\n";
                cout << "\t" << tpname << " = getelemptr " << ttp->arr_sym << ", 0\n";
                aststack.push(tpname);
            }
            else if (sym_type == 4) // ptr
            {
                PtrEntry *ttp = (PtrEntry *)tp;
                string tpname = "%" + to_string(tpname_count++);
                ans += "\t" + tpname + " = load " + ttp->ptr_sym + "\n";
                cout << "\t" << tpname << " = load " << ttp->ptr_sym << "\n";
                aststack.push(tpname);
            }

            else
            {
                aststack.push(ident);
            }
        }
        else //数组或指针
        {
            auto tp = symtab->QueryUp(ident);
            if (tp == nullptr)
            {
                cerr << "No declaration before" << endl;
                exit(1);
            }
            /*
            cout << ident << ": " << tp->ty << endl;
            cout<<"print param table"<<endl;
            for(int i = 0; i < arr_lval_param.size(); i++)
            {
                cout<<arr_lval_param[i]<<", "<<arr_lval_param_layer[i]<<endl;
            }
            */
            
            if (tp->ty == 3) //数组
            {
                ArrEntry *ttp = (ArrEntry *)tp;
                int tarlen = arr_lval_param.size();
                //int dim = ttp->arr_d;
                string lname = ttp->arr_sym;
                // cout<<"array dime: "<<dim<<", param dime: "<<tarlen<<endl;
                int jie = arr_lval_param_layer[tarlen-1];
                int cnt = 0;
                for(int i = tarlen-1; i >=0; i--)
                {
                    if(arr_lval_param_layer[i]==jie) cnt++;
                    else break;
                }
                for (int i = tarlen-cnt; i < tarlen - 1; i++)
                {
                    string tarname = arr_lval_param[i];
                    string tpname = "\%p" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                vector<int> tv;
                tv.clear();
                int tlen = ttp->arr_sym.length();
                string aaa = ttp->arr_sym.substr(1, tlen - 1);
                if (symtab->GetNextPtr(aaa, tv))
                {
                    symtab->InsertPtr(lname, lname, tv.size(), tv, "123");
                }
                bool tflag = false;

                if (tarlen > 0 && ttp->arr_d != tarlen)
                {
                    tflag = true;
                    string tarname = arr_lval_param[tarlen - 1];
                    string tpname = "\%p" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                else if (tarlen > 0 && ttp->arr_d == tarlen)    
                {
                    tflag = true;
                    string tarname = arr_lval_param[tarlen - 1];
                    string tpname = "\%q" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                if (tflag)
                {
                    vector<int> tv;
                    symtab->InsertPtr(lname, lname, 0, tv, "123");
                }
                for(int i = 0; i < cnt; i++)
                {
                    arr_lval_param.pop_back();
                    arr_lval_param_layer.pop_back();
                }
                

                param_is_arr = false;
                aststack.push(lname);
            }
            else //指针
            {
                PtrEntry *ttp = (PtrEntry *)tp;
                int tarlen = arr_lval_param.size();
                int jie = arr_lval_param_layer[tarlen-1];
                int cnt = 0;
                for(int i = tarlen-1; i >=0; i--)
                {
                    if(arr_lval_param_layer[i]==jie) cnt++;
                    else break;
                }
                //int dim = ttp->arr_d;
                // cout << "ptr dime: " << dim<< ", param dime: " << tarlen << endl;
                string lname = ttp->ptr_sym;
                string tpname = "\%p" + to_string(tpname_count++);
                cout << "\t" << tpname << " = load " << lname << endl;
                ans += "\t" + tpname + " = load " + lname + "\n";
                lname = tpname;
                tpname = "\%p" + to_string(tpname_count++);
                cout << 11111 << endl;
                string tarname = arr_lval_param[0];
                if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                {
                    auto tp2 = symtab->QueryUp(tarname);
                    string ttpname = "%" + to_string(tpname_count++);
                    VarEntry *ttp = (VarEntry *)tp2;
                    ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                    cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                    tarname = ttpname;
                }
                if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                {
                    string ttpname = "%" + to_string(tpname_count++);
                    ans += "\t" + ttpname + " = load " + tarname + "\n";
                    cout << "\t" << ttpname << " = load " << tarname << endl;
                    tarname = ttpname;
                }
                // cout<<tarname<<endl;
                cout << 22222 << endl;
                if (ttp->arr_d == 1)
                {
                    int tlen = tpname.length();
                    tpname = tpname.substr(2, tlen - 1);
                    tpname = "\%q" + tpname;
                }
                ans += "\t" + tpname + " = getptr " + lname + ", " + tarname + "\n";
                cout << "\t" << tpname << " = getptr " << lname << ", " << tarname << endl;
                lname = tpname;
                vector<int> tv;
                tv.clear();
                int tlen = ttp->ptr_sym.length();
                string aaa = ttp->ptr_sym.substr(1, tlen - 1);
                if (symtab->GetNextPtr(aaa, tv))
                {
                    symtab->InsertPtr(lname, lname, tv.size(), tv, "123");
                }
                bool tflag = false;

                // cout<<"len: "<<tarlen<<endl;
                for (int i = tarlen-cnt+1; i < tarlen - 1; i++)
                {
                    tflag = true;
                    string tarname = arr_lval_param[i];
                    string tpname = "\%p" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    cout << 33333 << endl;
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                cout << 44444 << endl;
                if (tarlen > 1 && ttp->arr_d != tarlen)
                {
                    tflag = true;
                    string tarname = arr_lval_param[tarlen - 1];
                    string tpname = "\%p" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    cout << 55555 << endl;
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                else if (tarlen > 1 && ttp->arr_d == tarlen)
                {
                    tflag = true;
                    cout << 66666 << endl;
                    string tarname = arr_lval_param[tarlen - 1];
                    string tpname = "\%q" + to_string(tpname_count++);
                    if (tarname[0] != '%' && !(tarname[0] >= '0' && tarname[0] <= '9'))
                    {
                        auto tp2 = symtab->QueryUp(tarname);
                        string ttpname = "%" + to_string(tpname_count++);
                        VarEntry *ttp = (VarEntry *)tp2;
                        ans += "\t" + ttpname + " = load " + ttp->var_sym + "\n";
                        cout << "\t" << ttpname << " = load " << ttp->var_sym << endl;
                        tarname = ttpname;
                    }
                    if (tarname[0] == '%' && (tarname[1] == 'p' || tarname[1] == 'q'))
                    {
                        string ttpname = "%" + to_string(tpname_count++);
                        ans += "\t" + ttpname + " = load " + tarname + "\n";
                        cout << "\t" << ttpname << " = load " << tarname << endl;
                        tarname = ttpname;
                    }
                    cout << 77777 << endl;
                    // cout<<tarname<<endl;
                    ans += "\t" + tpname + " = getelemptr " + lname + ", " + tarname + "\n";
                    cout << "\t" << tpname << " = getelemptr " << lname << ", " << tarname << endl;
                    lname = tpname;
                }
                if (tflag)
                {
                    vector<int> tv;
                    symtab->InsertPtr(lname, lname, 0, tv, "123");
                }
                for(int i = 0; i < cnt; i++)
                {
                    arr_lval_param.pop_back();
                    arr_lval_param_layer.pop_back();
                }
               
                param_is_arr = false;
                aststack.push(lname);
            }
        }
        
        if (is_arr)
        {
            is_arr = false;
            //arr_lval_param.clear();
            //arr_lval_param_layer.clear();
        }
        
        // PrintStack(aststack);
    }
};

class ExpArrAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp_arr;
    unique_ptr<BaseAST> exp;

    void Dump(string &ast) const override
    {
        cout << 1;
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (exp_arr != nullptr)
        {
            is_arr = true;
            if (is_cal_param)
                param_is_arr = true;
            exp_arr->dfs(ans, aststack, symtab);
            exp->dfs(ans, aststack, symtab);
            // cout<<3<<" "<<aststack.size()<<endl;
            genAST(ans, aststack, symtab);
            // PrintStack(aststack);
            //cout<<"push:"<<aststack.top()<<", "<<arrlayer<<endl;
            arr_lval_param.push_back(aststack.top());
            arr_lval_param_layer.push_back(arr_param_layer);
            aststack.pop();
            is_arr = true;
        }
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    int number;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    void Dump(string &ast) const override
    {
        cout << "PrimaryExpAST { ";
        ast += "PrimaryExpAST { ";
        if (exp != nullptr)
        {
            cout << "(";
            ast += "(";
            exp->Dump(ast);
            cout << ")";
            ast += ")";
        }
        else if (lval == nullptr)
        {
            cout << number;
            ast += to_string(number);
        }
        else
        {
            lval->Dump(ast);
        }
        cout << " }";
        ast += " }";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (exp != nullptr)
        {
            // cout<<"(";
            aststack.push("(");
            exp->dfs(ans, aststack, symtab);
            // cout<<")";
            aststack.push(")");
        }
        else if (lval == nullptr)
        {
            // cout<<number;
            aststack.push(to_string(number));
        }
        else
        {
            lval->dfs(ans, aststack, symtab);
        }
    }
};

class UnaryOpAST : public BaseAST
{
public:
    string unary_op; // + - !
    void Dump(string &ast) const override
    {
        cout << "UnaryOpAST { ";
        ast += "UnaryOpAST { ";
        cout << unary_op;
        ast += unary_op;
        cout << " }";
        ast += " }";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        // cout<<unary_op<<" ";
        aststack.push(unary_op + " 1");
    }
};

class AddExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> mul_exp;
    unique_ptr<BaseAST> add_exp;
    string add_op; //+ -, binary op
    void Dump(string &ast) const override
    {
        cout << "AddExpAST { ";
        ast += "AddExpAST { ";
        if (add_exp != nullptr)
        {
            add_exp->Dump(ast);
            cout << " " << add_op << " ";
            ast += " " + add_op + " ";
            mul_exp->Dump(ast);
        }
        else
        {
            mul_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (add_exp != nullptr)
        {
            aststack.push(add_op);
            // cout<<add_op<<" ";
            add_exp->dfs(ans, aststack, symtab);
            mul_exp->dfs(ans, aststack, symtab);
        }
        else
        {
            mul_exp->dfs(ans, aststack, symtab);
        }
    }
};

class MulExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> unary_exp;
    unique_ptr<BaseAST> mul_exp;
    string mul_op; //* / %, binary op
    void Dump(string &ast) const override
    {
        cout << "MulExpAST { ";
        ast += "MulExpAST { ";
        if (mul_exp != nullptr)
        {
            mul_exp->Dump(ast);
            cout << " " << mul_op << " ";
            ast += " " + mul_op + " ";
            unary_exp->Dump(ast);
        }
        else
        {
            unary_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (mul_exp != nullptr)
        {
            aststack.push(mul_op);
            cout << mul_op << " ";
            mul_exp->dfs(ans, aststack, symtab);
            unary_exp->dfs(ans, aststack, symtab);
        }
        else
        {
            unary_exp->dfs(ans, aststack, symtab);
        }
    }
};

class LOrExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> lor_exp;
    void Dump(string &ast) const override
    {
        cout << "LOrExpAST { ";
        ast += "LOrExpAST { ";
        if (lor_exp != nullptr)
        {
            lor_exp->Dump(ast);
            cout << " || ";
            ast += " || ";
            land_exp->Dump(ast);
        }
        else
        {
            land_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (exp_is_const == true)
        {
            if (lor_exp != nullptr)
            {
                aststack.push("||");
                lor_exp->dfs(ans, aststack, symtab);
                land_exp->dfs(ans, aststack, symtab);
            }
            else
            {
                land_exp->dfs(ans, aststack, symtab);
            }
            return;
        }

        if (lor_exp != nullptr)
        {
            bool flag = false;
            bool flag2 = false;
            bool flag3 = false;
            cout << "Layer: " << or_and_layer << endl;
            if (is_in_shortcut == false)
            {
                // cout<<"or1"<<endl;
                flag = true;
                in_and = false;
                is_in_shortcut = true;
                string start_name = "\%tp_then_" + to_string(shortcut_count);
                or_stack.push(start_name);
                or_stack_1.push(or_and_layer);
            }
            else if (in_and == true)
            {
                // cout<<"or2"<<endl;
                flag2 = true;
                in_and = false;
                string tpname = "\%tp_then_" + to_string(shortcut_count);
                or_stack.push(tpname);
                or_stack_1.push(or_and_layer);
            }
            else
            {
                if (or_stack_1.top() != or_and_layer)
                {
                    flag3 = true;
                    in_and = false;
                    string tpname = "\%tp_then_" + to_string(shortcut_count);
                    or_stack.push(tpname);
                    or_stack_1.push(or_and_layer);
                }
            }
            string result_name = "result_" + to_string(shortcut_count);
            string if_name = "\%tp_if_" + to_string(shortcut_count);
            string then_name = "\%tp_then_" + to_string(shortcut_count++);
            symtab->InsertVar(result_name, "@" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
            ans += "\t@" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32\n";
            ans += "\tstore 1, @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
            stack<string> fuzhu; //存一下当前栈里有的东西，然后算完短路求值以后再塞回去
            while (!aststack.empty())
            {
                fuzhu.push(aststack.top());
                aststack.pop();
            }

            aststack.push("==");
            lor_exp->dfs(ans, aststack, symtab);
            aststack.push("0");
            genAST(ans, aststack, symtab);
            string r_name = aststack.top();
            aststack.pop();
            // cout<<"Orsize: "<<or_stack.size()<<endl;
            if ((r_name[0] == '%' && r_name[1] != 'p' && r_name[1] != 'q') || isdigit(r_name[0]))
            {
                ans += "\tbr " + r_name + ", " + if_name + ", " + or_stack.top() + "\n";
            }
            else
            {
                auto tp = symtab->QueryUp(r_name);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans += "\t%" + tpname + " = load " + varsym + "\n";
                ans += "\tbr %" + tpname + ", " + if_name + ", " + or_stack.top() + "\n";
            }
            ans += if_name + ":\n";

            aststack.push("!=");
            land_exp->dfs(ans, aststack, symtab);
            aststack.push("0");
            genAST(ans, aststack, symtab);
            r_name = aststack.top();
            aststack.pop();
            auto tp = symtab->QueryUp(r_name);
            if (tp == nullptr)
            {
                ans += "\tstore " + r_name + ", @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
                ans += "\tjump " + then_name + "\n";
            }
            else //符号名 需要load以后再返回
            {
                string tpname = "%" + to_string(tpname_count++);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                ans += "\t" + tpname + " = load " + varsym + "\n";
                ans += "\tstore " + tpname + ", @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
                ans += "\tjump " + then_name + "\n";
            }
            ans += then_name + ":\n";

            while (!fuzhu.empty())
            {
                aststack.push(fuzhu.top());
                fuzhu.pop();
            }
            if (flag2)
            {
                or_stack.pop();
                or_stack_1.pop();
                in_and = true;
            }
            if (exp_is_const == false)
                aststack.push(result_name);
            else
                aststack.push(to_string(cal_val));
            // symtab->Print();
            if (flag)
            {
                is_in_shortcut = false;
                or_stack.pop();
                or_stack_1.pop();
            }
            if (flag3)
            {
                or_stack.pop();
                or_stack_1.pop();
            }
        }
        else
        {
            land_exp->dfs(ans, aststack, symtab);
            // cout<<aststack.size()<<endl;
        }
    }
};

class LAndExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> eq_exp;
    void Dump(string &ast) const override
    {
        cout << "LAndExpAST { ";
        ast += "LAndExpAST { ";
        if (land_exp != nullptr)
        {
            land_exp->Dump(ast);
            cout << " && ";
            ast += " && ";
            eq_exp->Dump(ast);
        }
        else
        {
            eq_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (exp_is_const == true)
        {
            if (land_exp != nullptr)
            {
                aststack.push("&&");
                land_exp->dfs(ans, aststack, symtab);
                eq_exp->dfs(ans, aststack, symtab);
            }
            else
            {
                eq_exp->dfs(ans, aststack, symtab);
            }
            return;
        }

        if (land_exp != nullptr)
        {
            cout << "Layer: " << or_and_layer << endl;
            bool flag = false;
            bool flag2 = false;
            bool flag3 = false;
            if (is_in_shortcut == false)
            {
                // cout<<"and1"<<endl;
                flag = true;
                in_and = true;
                is_in_shortcut = true;
                string start_name = "\%tp_then_" + to_string(shortcut_count);
                and_stack.push(start_name);
                and_stack_1.push(or_and_layer);
            }
            else if (in_and == false)
            {
                // cout<<"and2"<<endl;
                flag2 = true;
                in_and = true;
                string tpname = "\%tp_then_" + to_string(shortcut_count);
                and_stack.push(tpname);
                and_stack_1.push(or_and_layer);
            }
            else
            {
                if (and_stack_1.top() != or_and_layer)
                {
                    flag3 = true;
                    in_and = true;
                    string tpname = "\%tp_then_" + to_string(shortcut_count);
                    and_stack.push(tpname);
                    and_stack_1.push(or_and_layer);
                }
            }
            string result_name = "result_" + to_string(shortcut_count);
            string if_name = "\%tp_if_" + to_string(shortcut_count);
            string then_name = "\%tp_then_" + to_string(shortcut_count++);
            symtab->InsertVar(result_name, "@" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no));
            ans += "\t@" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + " = alloc i32\n";
            ans += "\tstore 0, @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
            stack<string> fuzhu; //存一下当前栈里有的东西，然后算完短路求值以后再塞回去
            while (!aststack.empty())
            {
                fuzhu.push(aststack.top());
                aststack.pop();
            }

            aststack.push("!=");
            land_exp->dfs(ans, aststack, symtab);
            aststack.push("0");
            genAST(ans, aststack, symtab);
            string r_name = aststack.top();
            aststack.pop();
            // cout<<"Andsize: "<<and_stack.size()<<endl;
            if ((r_name[0] == '%' && r_name[1] != 'p' && r_name[1] != 'q') || isdigit(r_name[0]))
            {
                ans += "\tbr " + r_name + ", " + if_name + ", " + and_stack.top() + "\n";
            }
            else
            {
                auto tp = symtab->QueryUp(r_name);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                string tpname = to_string(tpname_count++);
                ans += "\t%" + tpname + " = load " + varsym + "\n";
                ans += "\tbr %" + tpname + ", " + if_name + ", " + and_stack.top() + "\n";
            }
            ans += if_name + ":\n";

            aststack.push("!=");
            eq_exp->dfs(ans, aststack, symtab);
            aststack.push("0");
            genAST(ans, aststack, symtab);
            r_name = aststack.top();
            aststack.pop();
            auto tp = symtab->QueryUp(r_name);
            if (tp == nullptr)
            {
                ans += "\tstore " + r_name + ", @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
                ans += "\tjump " + then_name + "\n";
            }
            else //符号名 需要load以后再返回
            {
                string tpname = "%" + to_string(tpname_count++);
                VarEntry *ttp = (VarEntry *)tp;
                string varsym = ttp->var_sym;
                ans += "\t" + tpname + " = load " + varsym + "\n";
                ans += "\tstore " + tpname + ", @" + result_name + "_" + to_string(symtab->layer) + "_" + to_string(symtab->child_no) + "\n";
                ans += "\tjump " + then_name + "\n";
            }
            ans += then_name + ":\n";

            while (!fuzhu.empty())
            {
                aststack.push(fuzhu.top());
                fuzhu.pop();
            }
            if (flag2)
            {
                and_stack.pop();
                in_and = false;
                and_stack_1.pop();
            }
            if (exp_is_const == false)
                aststack.push(result_name);
            else
                aststack.push(to_string(cal_val));
            // symtab->Print();
            if (flag)
            {
                is_in_shortcut = false;
                and_stack.pop();
                and_stack_1.pop();
            }
            if (flag3)
            {
                and_stack.pop();
                and_stack_1.pop();
            }
        }
        else
        {
            eq_exp->dfs(ans, aststack, symtab);
        }
    }
};

class EqExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> eq_exp;
    string eq_op;
    void Dump(string &ast) const override
    {
        cout << "EqExpAST { ";
        ast += "EqExpAST { ";
        if (eq_exp != nullptr)
        {
            eq_exp->Dump(ast);
            // cout<<" "<<eq_op<<" ";
            ast += " " + eq_op + " ";
            rel_exp->Dump(ast);
        }
        else
        {
            rel_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (eq_exp != nullptr)
        {
            aststack.push(eq_op);
            // cout<<eq_op<<" ";
            eq_exp->dfs(ans, aststack, symtab);
            rel_exp->dfs(ans, aststack, symtab);
        }
        else
        {
            rel_exp->dfs(ans, aststack, symtab);
        }
    }
};

class RelExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> add_exp;
    string rel_op;
    void Dump(string &ast) const override
    {
        cout << "RelExpAST { ";
        ast += "RelExpAST { ";
        if (rel_exp != nullptr)
        {
            rel_exp->Dump(ast);
            cout << " " << rel_op << " ";
            ast += " " + rel_op + " ";
            add_exp->Dump(ast);
        }
        else
        {
            add_exp->Dump(ast);
        }
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        if (rel_exp != nullptr)
        {
            aststack.push(rel_op);
            // cout<<rel_op<<" ";
            rel_exp->dfs(ans, aststack, symtab);
            add_exp->dfs(ans, aststack, symtab);
        }
        else
        {
            add_exp->dfs(ans, aststack, symtab);
        }
    }
};

class ConditionAST : public BaseAST
{
public:
public:
    unique_ptr<BaseAST> lor_exp;
    void Dump(string &ast) const override
    {
        cout << "ConditionAST { ";
        ast += "ConditionAST { ";
        lor_exp->Dump(ast);
        cout << " }";
        ast += " }";
    }
    void dfs(string &ans, stack<string> &aststack, Symtab *symtab) const override
    {
        lor_exp->dfs(ans, aststack, symtab);
    }
};
