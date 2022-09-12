#include <iostream>
#include <string>
#include <memory>
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <cassert>
#include <typeinfo>
#include "koopa.h"
#include "def.hpp"
#include "localvartab.hpp"

using namespace std;

bool exist_call;
int space_for_param_more_than_8;
string reg_name[15] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
int reg_count = 0; //从零开始的寄存器生活（bushi）
int branch_label_count = 0;
LocalVarTable *FuncSpTable;
GlobalTable Gtable;
struct arrss
{
    vector<int> v;
    int cap;
    arrss(vector<int> vv, int cc) : v(vv), cap(cc) {}
};
unordered_map<string, struct arrss> arr_size; //每个函数入口处清零
// unordered_map<string,string> lval2reg;  //维护表达式左值与所在寄存器的一个关系

void Visit(const koopa_raw_program_t &program, std::string &ans)
{
    //库函数给一张空的表，存一下函数返回值类型和参数个数就好了
    string lib_func_name[8] = {"@getint", "@getch", "@getarray", "@putint", "@putch", "@putarray", "@starttime", "@stoptime"};
    bool lib_func_ty[8] = {false, false, false, true, true, true, true, true};
    int lib_func_params[8] = {0, 0, 1, 1, 1, 2, 0, 0};
    for (int i = 0; i < 8; i++)
    {
        LocalVarTable *sp_table = new LocalVarTable(lib_func_name[i], 0, 0, lib_func_params[i], lib_func_ty[i]);
        Gtable.Insert_functable(lib_func_name[i], sp_table);
    }
    Visit(program.values, ans);
    Visit(program.funcs, ans);
    Gtable.DeleteAll(); //释放函数表的空间
}

void Visit(const koopa_raw_slice_t &slice, std::string &ans)
{
    //函数名 globl f1, f2部分
    // cout<<slice.kind<<" "<<slice.len<<endl;
    if (slice.kind == KOOPA_RSIK_BASIC_BLOCK)
    {
        // int len = slice.len;
        /*
        for(size_t i = 0; i < len; i++)
        {
            auto ptr = slice.buffer[i];
            const char* bbname = reinterpret_cast<koopa_raw_basic_block_t>(ptr)->name;
            cout<<"name: "<<bbname<<endl;
        }
        */
    }
    if (slice.kind == KOOPA_RSIK_FUNCTION)
    {
        // int len = slice.len;
        /*
        for(size_t i = 0; i < slice.len; i++)
        {
            auto ptr = slice.buffer[i];
            auto f_name = reinterpret_cast<koopa_raw_function_t>(ptr)->ty->data.function.ret->tag;
            cout<<f_name<<" ";
        }
        cout<<endl;
        */
    }
    if (slice.kind == KOOPA_RSIK_VALUE)
    {
        // int len = slice.len;
    }
    for (size_t i = 0; i < slice.len; i++)
    {
        auto ptr = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr), ans);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), ans);
            break;
        case KOOPA_RSIK_VALUE:
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr), ans);
            break;
        default:
            assert(false);
        }
    }
}

//第一遍，统计需要分配多少栈帧
int Pass1(const koopa_raw_function_t &func)
{
    int tot = 0;
    space_for_param_more_than_8 = 0;
    exist_call = false;
    koopa_raw_slice_t slice = func->bbs;
    int len = slice.len;
    // cout<<"Len: "<<len<<endl;
    for (size_t i = 0; i < len; i++)
    {
        auto ptr = slice.buffer[i];
        // cout<<slice.kind<<endl;
        if (slice.kind == KOOPA_RSIK_BASIC_BLOCK)
        {
            auto bb = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
            auto insts = bb->insts;
            int slen = insts.len;
            for (size_t j = 0; j < slen; j++)
            {
                auto ptr2 = insts.buffer[j];
                if (insts.kind == KOOPA_RSIK_VALUE)
                {
                    auto ptr3 = reinterpret_cast<koopa_raw_value_t>(ptr2);
                    auto ukind = ptr3->kind.tag;
                    // cout<<"kind: "<<ukind<<endl;
                    //以后扩展更多指令
                    if (ukind == KOOPA_RVT_CALL)
                    {
                        exist_call = true;
                        auto call_inst = ptr3->kind.data.call;
                        // cout<<"arglen:"<<call_inst.args.len<<endl;
                        // cout<<call_inst.callee->name<<endl;
                        space_for_param_more_than_8 = max(max(0, (int)call_inst.args.len - 8), space_for_param_more_than_8);
                    }
                    if (ukind != KOOPA_RVT_RETURN && ukind != KOOPA_RVT_STORE &&
                        ukind != KOOPA_RVT_ALLOC && ukind != KOOPA_RVT_JUMP &&
                        ukind != KOOPA_RVT_GLOBAL_ALLOC && ukind != KOOPA_RVT_BRANCH)
                    {
                        ++tot;
                    }
                    if (ukind == KOOPA_RVT_ALLOC)
                    {
                        auto cnt = ptr3->ty->data.array;
                        int maxn = 1;
                        bool tpflag = false;
                        vector<int> tp;
                        tp.clear();
                        while (cnt.base->tag == KOOPA_RTT_ARRAY)
                        {
                            tpflag = true;
                            int len = cnt.base->data.array.len;
                            tp.push_back(len);
                            maxn *= len;
                            cnt = cnt.base->data.array;
                        }
                        tot += maxn;
                        if (tpflag) // array alloc
                        {
                            arr_size.insert(make_pair(ptr3->name, arrss(tp, maxn)));
                        }
                    }
                }
            }
        }
    }
    if (exist_call)
        tot++;
    tot += space_for_param_more_than_8;
    /*
    for(auto it = arr_size.begin();it!=arr_size.end();it++)
    {
        cout<<it->first<<": ";
        for(auto iit = it->second.v.begin(); iit!=it->second.v.end();iit++)
        {
            cout<<*iit<<" ";
        }
        cout<<endl;
    }
    */

    // cout<<"tot "<<tot<<endl;
    return tot;
}

//访问函数
void Visit(const koopa_raw_function_t &func, std::string &ans)
{
    arr_size.clear();
    int params_len = func->params.len;
    for (int i = 0; i < params_len; i++)
    {
        auto tp = func->params.buffer[i];
        auto tp2 = reinterpret_cast<koopa_raw_function_t>(tp);
        auto tp3 = tp2->ty->tag;
        if (tp3 == KOOPA_RTT_POINTER)
        {
            vector<int> v;
            v.push_back(0);
            auto tp4 = tp2->ty->data.pointer.base;
            while (tp4->tag == KOOPA_RTT_ARRAY)
            {
                v.push_back(tp4->data.array.len);
                tp4 = tp4->data.array.base;
            }
            string nname(tp2->name);
            int nlen = nname.length();
            nname = nname.substr(1, nlen - 1);
            nname = "%" + nname;
            arr_size.insert(make_pair(nname, arrss(v, 1)));
        }
    }
    auto ts = func->ty->data.function.ret->tag;
    bool is_void = false;
    if (ts == KOOPA_RTT_UNIT)
    {
        is_void = true;
    }
    else if (ts == KOOPA_RTT_INT32)
    {
        is_void = false;
    }
    auto check_ = func->bbs.len;
    if (check_ == 0)
    {
        return;
    }
    cout << endl;
    ans += "\n";
    cout << "\t.text\n";
    ans += "\t.text\n";
    int sp_space = Pass1(func) * 4;
    sp_space = sp_space % 16 ? (sp_space / 16 + 1) * 16 : sp_space;
    // cout<<"Size: "<<sp_space<<endl;
    const char *f_name = func->name;
    string s = (f_name);
    int s_size = s.length();
    cout << "\t.globl " << s.substr(1, s_size - 1) << endl;
    ans += "\t.globl " + s.substr(1, s_size - 1) + "\n";
    cout << s.substr(1, s_size - 1) << ":\n";
    ans += s.substr(1, s_size - 1) + ":\n";
    if (sp_space < 2048)
    {
        cout << "\taddi sp, sp, -" << sp_space << endl;
        ans += "\taddi sp, sp, -" + to_string(sp_space) + "\n";
    }
    else
    {
        cout << "\tli t0, -" << sp_space << endl;
        ans += "\tli t0, -" + to_string(sp_space) + "\n";
        cout << "\tadd sp, sp, t0\n";
        ans += "\tadd sp, sp, t0\n";
    }
    if (exist_call)
    {
        int a0 = sp_space - 4;
        if (a0 > 2047)
        {
            cout << "\tli t0, " << a0 << endl;
            ans += "\tli t0, " + to_string(a0) + "\n";
            cout << "\tadd t0, t0, sp\n";
            ans += "\tadd t0, t0, sp\n";
            cout << "\tsw ra, 0(t0)\n";
            ans += "\tsw ra, 0(t0)\n";
        }
        else
        {
            cout << "\tsw ra, " << a0 << "(sp)\n";
            ans += "\tsw ra, " + to_string(a0) + "(sp)\n";
        }
    }
    LocalVarTable *sp_table = new LocalVarTable(s, space_for_param_more_than_8 * 4, sp_space, func->params.len, is_void);
    Gtable.Insert_functable(func->name, sp_table);
    FuncSpTable = sp_table;
    // auto params = func->params;
    Visit(func->bbs, ans);
    // FuncSpTable->Print();
}

//对每一个基本块打印它的名字
void Visit(const koopa_raw_basic_block_t &bb, std::string &ans)
{
    const char *b_name = bb->name;
    string r_name = string(b_name);
    r_name = r_name.substr(1, r_name.length() - 1) + ":";
    cout << r_name << endl;
    ans += r_name + "\n";
    // auto params = bb->params;
    Visit(bb->insts, ans); /*  */
}

//对每一句的操作进行判断
void Visit(const koopa_raw_value_t &value, std::string &ans)
{
    const auto &kind = value->kind;
    // cout<<"kind: "<<kind.tag<<endl;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN: // 16
        Visit(kind.data.ret, ans);
        break;
    case KOOPA_RVT_INTEGER: // 0
        Visit(kind.data.integer, ans);
        break;
    case KOOPA_RVT_BINARY: // 12
        Visit(kind.data.binary, ans);
        break;
    case KOOPA_RVT_ALLOC: // 6
        Visit(value->name, ans);
        break;
    case KOOPA_RVT_LOAD: // 8
        Visit(kind.data.load, ans);
        break;
    case KOOPA_RVT_STORE: // 9
        Visit(value, kind.data.store, ans);
        break;
    case KOOPA_RVT_JUMP: // 14
        Visit(kind.data.jump, ans);
        break;
    case KOOPA_RVT_BRANCH: // 13
        Visit(kind.data.branch, ans);
        break;
    case KOOPA_RVT_CALL: // 15
        Visit(kind.data.call, ans);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC: // 7
        // cout<<value->ty->data.pointer.base->data.array.len<<endl;
        Visit(kind.data.global_alloc, value->name, ans);
        break;
    case KOOPA_RVT_GET_ELEM_PTR: // 11
        if (value->name == nullptr)
        {
            Visit("This is a nullptr", kind.data.get_elem_ptr, ans);
        }
        else
        {
            Visit(value->name, kind.data.get_elem_ptr, ans);
        }
        break;
    case KOOPA_RVT_GET_PTR: // 10
        Visit(value->name, kind.data.get_ptr, ans);
        break;
    default:
        cout << "kind: " << kind.tag << endl;
        assert(false);
    }
}

//处理getptr指令
void Visit(std::string ptrname, const koopa_raw_get_ptr_t &getelemptr, std::string &ans)
{
    int ind = 0, off_mul = 0;
    bool is_load = false;
    auto tp = getelemptr.index->kind;
    {
        switch (tp.tag)
        {
        case KOOPA_RVT_INTEGER:
            ind = tp.data.integer.value;
            break;
        case KOOPA_RVT_LOAD:
            is_load = true;
            break;
        case KOOPA_RVT_BINARY:
            is_load = true;
            break;
        default:
            cout << "tag:" << tp.tag << endl;
            assert(false);
        }
    }
    char str[10];
    sprintf(str, "%p", &getelemptr.src->kind.data.load);
    string addr(str);
    int loc = FuncSpTable->Query(addr);
    if (loc == -1) //全局数组名
    {
        cout << "invalid!" << endl;
    }
    else
    {
        vector<int> tpv;
        tpv.clear();
        // FuncSpTable->PrintPtr();
        // cout<<addr<<endl;
        off_mul = FuncSpTable->GetNextPtrSize(addr, tpv) * 4;
        // cout << "offset: " << off_mul << endl;
        FuncSpTable->InsertPtr(ptrname, tpv);
        if (loc >= 2048)
        {
            ans += "\tli t0, " + to_string(loc) + "\n";
            cout << "\tli t0, " << loc << endl;
            ans += "\tadd t0, t0, sp\n";
            cout << "\tadd t0, t0, sp\n";
            ans += "\tlw t0, 0(t0)\n";
            cout << "\tlw t0, 0(t0)\n";
        }
        else
        {
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
            cout << "\tlw t0, " << loc << "(sp)\n";
        }
    }
    if (is_load)
    {
        char str[10];
        sprintf(str, "%p", &getelemptr.index->kind.data);
        int loc = FuncSpTable->Query(string(str));
        cout << loc << endl;
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t1, 0(t2)\n";
            cout << "\tlw t1, 0(t2)\n";
        }
        else
        {
            ans += "\tlw t1, " + to_string(loc) + "(sp)\n";
            cout << "\tlw t1, " << loc << "(sp)\n";
        }
        ans += "\tli t2, " + to_string(off_mul) + "\n";
        cout << "\tli t2, " << off_mul << endl;
        ans += "\tmul t1, t1, t2\n";
        cout << "\tmul t1, t1, t2\n";
        ans += "\tadd t0, t0, t1\n";
        cout << "\tadd t0, t0, t1\n";
    }
    else
    {
        GenOffset(ind, off_mul, ans);
    }
    int newloc = FuncSpTable->cntsp;
    FuncSpTable->cntsp += 4;
    FuncSpTable->Insert(ptrname, newloc);
    if (IntOverflow(newloc, "t2", "sp", ans))
    {
        ans += "\tsw t0, 0(t2)\n";
        cout << "\tsw t0, 0(t2)\n";
    }
    else
    {
        ans += "\tsw t0, " + to_string(newloc) + "(sp)\n";
        cout << "\tsw t0, " << newloc << "(sp)\n";
    }
    // FuncSpTable->Print();
}

//处理getelemptr指令
void Visit(string ptrname, const koopa_raw_get_elem_ptr_t &getelemptr, std::string &ans)
{
    bool fflag = false;
    char pstr[10];
    if (ptrname == "This is a nullptr")
    {
        fflag = true;
    }
    sprintf(pstr, "%p", &getelemptr);
    // if(!fflag) cout<<ptrname<<endl;
    int ind = 0, off_mul = 0;
    bool is_load = false;
    auto tp = getelemptr.index->kind;
    {
        switch (tp.tag)
        {
        case KOOPA_RVT_INTEGER:
            ind = tp.data.integer.value;
            break;
        case KOOPA_RVT_LOAD:
            is_load = true;
            break;
        case KOOPA_RVT_BINARY:
            is_load = true;
            break;
        default:
            cout << "tag:" << tp.tag << endl;
            assert(false);
        }
    }
    string name = getelemptr.src->name;
    // cout << "Getelemptr, name:" << name << endl;
    int loc = FuncSpTable->Query(name);
    if (loc == -1) //全局数组名
    {
        vector<int> tpv;
        tpv.clear();
        off_mul = Gtable.GetNextPtrSize(name, tpv) * 4;
        int nlen = name.length();
        name = name.substr(1, nlen - 1);
        FuncSpTable->InsertPtr(ptrname, tpv);
        ans += "\tla t0, " + name + "\n";
        cout << "\tla t0, " << name << endl;
    }
    else
    {
        vector<int> tpv;
        tpv.clear();
        off_mul = FuncSpTable->GetNextPtrSize(name, tpv) * 4;
        if (!fflag)
            FuncSpTable->InsertPtr(ptrname, tpv);
        if ((name[1] == 'p' || name[1] == 'q') && (name[2] >= '0' && name[2] <= '9'))
        {
            if (loc >= 2048)
            {
                ans += "\tli t0, " + to_string(loc) + "\n";
                cout << "\tli t0, " << loc << "\n";
                ans += "\tadd t0, t0, sp\n";
                cout << "\tadd t0, t0, sp\n";
                ans += "\tlw t0, 0(t0)\n";
                cout << "\tlw t0, 0(t0)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)\n";
            }
        }
        else
        {
            if (loc >= 2048)
            {
                ans += "\tli t0, " + to_string(loc) + "\n";
                cout << "\tli t0, " << loc << endl;
                ans += "\tadd t0, sp, t0\n";
                cout << "\tadd t0, sp, t0\n";
            }
            else
            {
                ans += "\taddi t0, sp, " + to_string(loc) + "\n";
                cout << "\taddi t0, sp, " << loc << endl;
            }
        }
    }
    if (is_load)
    {
        char str[10];
        sprintf(str, "%p", &getelemptr.index->kind.data);
        int loc = FuncSpTable->Query(string(str));
        cout << loc << endl;
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t1, 0(t2)\n";
            cout << "\tlw t1, 0(t2)\n";
        }
        else
        {
            ans += "\tlw t1, " + to_string(loc) + "(sp)\n";
            cout << "\tlw t1, " << loc << "(sp)\n";
        }
        ans += "\tli t2, " + to_string(off_mul) + "\n";
        cout << "\tli t2, " << off_mul << endl;
        ans += "\tmul t1, t1, t2\n";
        cout << "\tmul t1, t1, t2\n";
        ans += "\tadd t0, t0, t1\n";
        cout << "\tadd t0, t0, t1\n";
    }
    else
    {
        GenOffset(ind, off_mul, ans);
    }
    int newloc = FuncSpTable->cntsp;
    FuncSpTable->cntsp += 4;
    if (!fflag)
        FuncSpTable->Insert(ptrname, newloc);
    else
        FuncSpTable->Insert(string(pstr), newloc);
    if (IntOverflow(newloc, "t2", "sp", ans))
    {
        ans += "\tsw t0, 0(t2)\n";
        cout << "\tsw t0, 0(t2)\n";
    }
    else
    {
        ans += "\tsw t0, " + to_string(newloc) + "(sp)\n";
        cout << "\tsw t0, " << newloc << "(sp)\n";
    }
}

//产生数组元素偏移量计算的函数，可以进行移位优化代替乘法
void GenOffset(int a, int b, std::string &ans) //输出产生a*b的式子
{
    ans += "\tli t1, " + to_string(a) + "\n";
    cout << "\tli t1, " << a << endl;
    //之后用移位运算来进行优化
    ans += "\tli t2, " + to_string(b) + "\n";
    cout << "\tli t2, " << b << endl;
    ans += "\tmul t1, t1, t2\n";
    cout << "\tmul t1, t1, t2\n";
    ans += "\tadd t0, t0, t1\n";
    cout << "\tadd t0, t0, t1\n";
}

//对全局初始化的aggregate递归产生word
void GenAggr(const koopa_raw_value_kind_t &g, std::string &ans)
{
    auto tp = g.data.aggregate.elems;
    for (int i = 0; i < tp.len; i++)
    {
        auto tp1 = reinterpret_cast<koopa_raw_value_t>(tp.buffer[i]);
        if (tp1->kind.tag == KOOPA_RVT_INTEGER)
        {
            ans += "\t.word " + to_string(tp1->kind.data.integer.value) + "\n";
            cout << "\t.word " << tp1->kind.data.integer.value << endl;
        }
        else
        {
            GenAggr(tp1->kind, ans);
        }
    }
}

//处理global alloc
void Visit(const koopa_raw_global_alloc_t &g_alloc, std::string alloc_name, std::string &ans)
{
    auto tp1 = g_alloc.init->kind;
    ans += "\t.data\n\t.globl ";
    int n_len = alloc_name.length();
    alloc_name = alloc_name.substr(1, n_len - 1);
    ans += alloc_name + "\n" + alloc_name + ":\n";
    cout << "\t.data\n\t.globl " << alloc_name << endl
         << alloc_name << ":\n";
    if (tp1.tag == KOOPA_RVT_ZERO_INIT)
    {
        int ttag = g_alloc.init->ty->tag;
        if (ttag == KOOPA_RTT_INT32)
        {
            ans += "\t.zero 4\n";
            cout << "\t.zero 4\n";
        }
        else if (ttag == KOOPA_RTT_ARRAY)
        {
            int tlen = g_alloc.init->ty->data.array.len;
            auto cnt = g_alloc.init->ty->data.array;
            vector<int> tp;
            tp.clear();
            tp.push_back(tlen);
            while (cnt.base->tag == KOOPA_RTT_ARRAY)
            {
                int len = cnt.base->data.array.len;
                cnt = cnt.base->data.array;
                tp.push_back(len);
                tlen *= len;
            }
            ans += "\t.zero " + to_string(4 * tlen) + "\n";
            cout << "\t.zero " << 4 * tlen << endl;
            Gtable.Insert_globalptr("@" + alloc_name, tp);
        }
        else
        {
            cout << "tag: " << ttag << endl;
            assert(false);
        }
    }
    else if (tp1.tag == KOOPA_RVT_INTEGER)
    {
        auto tp2 = g_alloc.init->kind.data.integer.value;
        ans += "\t.word " + to_string(tp2) + "\n";
        cout << "\t.word " << tp2 << endl;
    }
    else if (tp1.tag == KOOPA_RVT_AGGREGATE)
    {
        auto cnt = g_alloc.init->ty->data.array;
        vector<int> v;
        v.push_back(cnt.len);
        while (cnt.base->tag == KOOPA_RTT_ARRAY)
        {
            int len = cnt.base->data.array.len;
            cnt = cnt.base->data.array;
            v.push_back(len);
        }
        GenAggr(g_alloc.init->kind, ans);
        Gtable.Insert_globalptr("@" + alloc_name, v);
    }
    else
    {
        cout << tp1.tag << endl;
        assert(false);
    }
    ans += "\n";
    cout << endl;
    Gtable.Insert_globalvar("@" + alloc_name);
}

//处理call指令
void Visit(const koopa_raw_call_t &call, std::string &ans)
{
    string callee_name = call.callee->name;
    auto target = Gtable.Query_functable(callee_name);
    int nlen = callee_name.length();
    callee_name = callee_name.substr(1, nlen - 1);
    int p_len = target->para_num;
    for (int i = 0; i < p_len && i < 8; i++)
    {
        string reg_name = "a" + to_string(i);
        // auto tp = call.callee->params.buffer[i];
        // const char* f_name = reinterpret_cast<koopa_raw_function_t>(tp)->name;
        auto ttp = call.args.buffer[i];
        auto p_kind = reinterpret_cast<koopa_raw_value_t>(ttp)->kind;
        // printf("456:%p %p\n",ttp,&reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
        char str[10];
        int loc;
        // cout<<"tag:"<<p_kind.tag<<" ";
        switch (p_kind.tag)
        {
        case KOOPA_RVT_INTEGER:
            ans += "\tli " + reg_name + ", " + to_string(p_kind.data.integer.value) + "\n";
            cout << "\tli " << reg_name << ", " << p_kind.data.integer.value << endl;
            break;

        case KOOPA_RVT_LOAD:
            cout << 11 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw " + reg_name + ", 0(t2)\n";
                cout << "\tlw " << reg_name << ", 0(t2)\n";
            }
            else
            {
                ans += "\tlw " + reg_name + ", " + to_string(loc) + "(sp)\n";
                cout << "\tlw " << reg_name << ", " << loc << "(sp)" << endl;
            }
            break;

        case KOOPA_RVT_BINARY:
            cout << 12 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw " + reg_name + ", 0(t2)\n";
                cout << "\tlw " << reg_name << ", 0(t2)\n";
            }
            else
            {
                ans += "\tlw " + reg_name + ", " + to_string(loc) + "(sp)\n";
                cout << "\tlw " << reg_name << ", " << loc << "(sp)" << endl;
            }
            break;

        case KOOPA_RVT_CALL:
            cout << 13 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw " + reg_name + ", 0(t2)\n";
                cout << "\tlw " << reg_name << ", 0(t2)\n";
            }
            else
            {
                ans += "\tlw " + reg_name + ", " + to_string(loc) + "(sp)\n";
                cout << "\tlw " << reg_name << ", " << loc << "(sp)" << endl;
            }
            break;

        case KOOPA_RVT_GET_ELEM_PTR:
            cout << 14 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw " + reg_name + ", 0(t2)\n";
                cout << "\tlw " << reg_name << ", 0(t2)\n";
            }
            else
            {
                ans += "\tlw " + reg_name + ", " + to_string(loc) + "(sp)\n";
                cout << "\tlw " << reg_name << ", " << loc << "(sp)" << endl;
            }
            break;

        case KOOPA_RVT_GET_PTR:
            cout << 15 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw " + reg_name + ", 0(t2)\n";
                cout << "\tlw " << reg_name << ", 0(t2)\n";
            }
            else
            {
                ans += "\tlw " + reg_name + ", " + to_string(loc) + "(sp)\n";
                cout << "\tlw " << reg_name << ", " << loc << "(sp)" << endl;
            }
            break;
        default:
            cout << "tag: " << p_kind.tag << endl;
            assert(false);
        }
    }
    for (int i = 8; i < p_len; i++)
    {
        auto ttp = call.args.buffer[i];
        string par_loc = to_string((i - 8) * 4) + "(sp)";
        auto p_kind = reinterpret_cast<koopa_raw_value_t>(ttp)->kind;
        char str[10];
        int loc;
        switch (p_kind.tag)
        {
        case KOOPA_RVT_INTEGER:
            ans += "\tli t0, " + to_string(p_kind.data.integer.value) + "\n";
            cout << "\tli t0, " << p_kind.data.integer.value << endl;
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        case KOOPA_RVT_LOAD:
            cout << 11 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)" << endl;
            }
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        case KOOPA_RVT_BINARY:
            cout << 12 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)" << endl;
            }
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        case KOOPA_RVT_CALL:
            cout << 13 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)" << endl;
            }
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        case KOOPA_RVT_GET_ELEM_PTR:
            cout << 14 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)" << endl;
            }
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        case KOOPA_RVT_GET_PTR:
            cout << 15 << endl;
            sprintf(str, "%p", &reinterpret_cast<koopa_raw_value_t>(ttp)->kind.data);
            loc = FuncSpTable->Query(string(str));
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)" << endl;
            }
            ans += "\tsw t0, " + par_loc + "\n";
            cout << "\tsw t0, " << par_loc << endl;
            break;

        default:
            cout << "tag: " << p_kind.tag << endl;
            assert(false);
        }
    }
    ans += "\tcall " + callee_name + "\n";
    cout << "\tcall " << callee_name << endl;
    // void函数无需在符号表里插入返回值
    auto s_table = Gtable.Query_functable("@" + callee_name);
    if (s_table->is_void == false)
    {
        char st[10];
        sprintf(st, "%p", &call);
        int loc1 = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        FuncSpTable->Insert(st, loc1);
        if (IntOverflow(loc1, "t2", "sp", ans))
        {
            ans += "\tsw a0, 0(t2)\n";
            cout << "\tsw a0, 0(t2)\n";
        }
        else
        {
            ans += "\tsw a0, " + to_string(loc1) + "(sp)\n";
            cout << "\tsw a0, " << loc1 << "(sp)" << endl;
        }
    }
}

//处理jump指令
void Visit(const koopa_raw_jump_t &jump, std::string &ans)
{
    // cout<<jump.target->name<<endl;
    string r_name = string(jump.target->name);
    r_name = r_name.substr(1, r_name.length() - 1);
    ans += "\tj " + r_name + "\n";
    cout << "\tj " << r_name << endl;
}

//处理branch指令
void Visit(const koopa_raw_branch_t &br, std::string &ans)
{
    /*
    cout<<br.true_bb->name<<" ";
    cout<<br.false_bb->name<<" ";
    cout<<br.cond->kind.tag<<endl;
    */
    string name1, name2;
    name1 = string(br.true_bb->name);
    name1 = name1.substr(1, name1.length() - 1);
    name2 = string(br.false_bb->name);
    name2 = name2.substr(1, name2.length() - 1);
    char str[10];
    int loc;
    switch (br.cond->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        ans += "\tli t0, " + to_string(br.cond->kind.data.integer.value) + "\n";
        cout << "\tli t0, " << br.cond->kind.data.integer.value << endl;
        break;

    case KOOPA_RVT_BINARY:
        sprintf(str, "%p", &br.cond->kind.data.binary);
        loc = FuncSpTable->Query(string(str));
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        break;

    case KOOPA_RVT_LOAD:
        sprintf(str, "%p", &br.cond->kind.data.load);
        loc = FuncSpTable->Query(string(str));
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        break;
    case KOOPA_RVT_CALL:
        sprintf(str, "%p", &br.cond->kind.data.load);
        loc = FuncSpTable->Query(string(str));
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        break;
    default:
        cout << br.cond->kind.tag << endl;
        assert(false);
    }
    string bl = "branch_label_" + to_string(branch_label_count++);
    ans += "\tbnez t0, " + bl + "\n";
    cout << "\tbnez t0, " << bl << endl;
    ans += "\tj " + name2 + "\n";
    cout << "\tj " << name2 << endl;
    ans += bl + ":\n";
    cout << bl << ":\n";
    ans += "\tla t0, " + name1 + "\n";
    cout << "\tla t0, " << name1 << endl;
    ans += "\tjr t0\n";
    cout << "\tjr t0\n";
}

//处理load指令
void Visit(const koopa_raw_load_t &load, std::string &ans)
{
    auto tag = load.src->kind.tag; //要加载的源的类型，alloc/binary
    if (tag == KOOPA_RVT_ALLOC)
    {
        // cout<<21<<endl;
        string name = load.src->name;
        int loc1 = FuncSpTable->Query(name);
        if (IntOverflow(loc1, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc1 << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc1) + "(sp)\n";
        }
        int loc2 = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        if (IntOverflow(loc2, "t2", "sp", ans))
        {
            ans += "\tsw t0, 0(t2)\n";
            cout << "\tsw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tsw t0, " << loc2 << "(sp)" << endl;
            ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
        }
        char str[10];
        sprintf(str, "%p", &load);
        string addr(str);
        FuncSpTable->Insert(addr, loc2);
        if (FuncSpTable->IsArr(name))
        {
            vector<int> v;
            FuncSpTable->GetNextPtrSize(name, v);
            FuncSpTable->InsertPtr(addr, v);
        }
    }
    else if (tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        // cout<<22<<endl;
        string name = load.src->name;
        int n_len = name.length();
        name = name.substr(1, n_len - 1);
        ans += "\tla t0, " + name + "\n";
        cout << "\tla t0, " << name << endl;
        ans += "\tlw t0, 0(t0)\n";
        cout << "\tlw t0, 0(t0)" << endl;
        int loc2 = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        if (IntOverflow(loc2, "t2", "sp", ans))
        {
            ans += "\tsw t0, 0(t2)\n";
            cout << "\tsw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tsw t0, " << loc2 << "(sp)" << endl;
            ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
        }
        char str[10];
        sprintf(str, "%p", &load);
        string addr(str);
        FuncSpTable->Insert(addr, loc2);
    }
    else if (tag == KOOPA_RVT_GET_ELEM_PTR)
    {
        // cout<<23<<endl;
        string name = load.src->name;
        int loc = FuncSpTable->Query(name);
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        ans += "\tlw t0, 0(t0)\n";
        cout << "\tlw t0, 0(t0)\n";
        int loc2 = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        if (IntOverflow(loc2, "t2", "sp", ans))
        {
            ans += "\tsw t0, 0(t2)\n";
            cout << "\tsw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tsw t0, " << loc2 << "(sp)" << endl;
            ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
        }
        char str[10];
        sprintf(str, "%p", &load);
        string addr(str);
        FuncSpTable->Insert(addr, loc2);
        // FuncSpTable->Print();
    }
    else if (tag == KOOPA_RVT_GET_PTR)
    {
        // cout<<24<<endl;
        // cout<<"getptr:";
        // cout<<load.src->name<<endl;
        string name = load.src->name;
        int loc = FuncSpTable->Query(name);
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        ans += "\tlw t0, 0(t0)\n";
        cout << "\tlw t0, 0(t0)\n";
        int loc2 = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        if (IntOverflow(loc2, "t2", "sp", ans))
        {
            ans += "\tsw t0, 0(t2)\n";
            cout << "\tsw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tsw t0, " << loc2 << "(sp)" << endl;
            ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
        }
        char str[10];
        sprintf(str, "%p", &load);
        string addr(str);
        FuncSpTable->Insert(addr, loc2);
    }
    else
    {
        cout << "tag: " << tag << endl;
        assert(false);
    }
}

//辅助生成store ptr指令的函数
bool Store_Ptr(std::string name, std::string &ans)
{
    //是指针，需要load
    if (FuncSpTable->IsArr(name))
    {
        int loc = FuncSpTable->Query(name);
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t1, 0(t2)\n";
            cout << "\tlw t1, 0(t2)\n";
        }
        else
        {
            ans += "\tlw t1, " + to_string(loc) + "(sp)\n";
            cout << "\tlw t1, " << loc << "(sp)\n";
        }
        ans += "\tsw t0, 0(t1)\n";
        cout << "\tsw t0, 0(t1)\n";
        return true;
    }
    //是普通变量
    else
    {
        return false;
    }
}

//处理store指令
void Visit(const koopa_raw_value_t &value, const koopa_raw_store_t &store, std::string &ans)
{
    koopa_raw_value_t dest = store.dest;
    koopa_raw_value_t val = store.value;
    string name = dest->name;
    // cout<<name<<", ";
    // cout<<"type:"<<val->kind.tag<<endl;
    //需要在查询name的时候判断一下这是不是一个指针类型，如果是的话需要改变方法
    if (val->kind.tag == KOOPA_RVT_BINARY) // 12
    {
        cout << 31 << endl;
        char str[10];
        sprintf(str, "%p", &val->kind.data.binary);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            cout << "\tlw t0, " << loc << "(sp)" << endl;
            ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
        }
        if (Store_Ptr(name, ans))
            return;
        loc = FuncSpTable->Query(name);
        if (loc != -1)
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tsw t0, 0(t2)\n";
                cout << "\tsw t0, 0(t2)\n";
            }
            else
            {
                cout << "\tsw t0, " << loc << "(sp)" << endl;
                ans += "\tsw t0, " + to_string(loc) + "(sp)\n";
            }
        }
        else
        {
            int l = name.length();
            string tname = name.substr(1, l - 1);
            ans += "\tla t1, " + tname + "\n";
            cout << "\tla t1, " << tname << endl;
            ans += "\tsw t0, 0(t1)\n";
            cout << "\tsw t0, 0(t1)\n";
        }
    }
    else if (val->kind.tag == KOOPA_RVT_INTEGER) // 0
    {
        //万一，这要是一个全局变量呢？
        cout << 32 << endl;
        int val1 = val->kind.data.integer.value;
        cout << "\tli t0, " << val1 << endl;
        ans += "\tli t0, " + to_string(val1) + "\n";
        if (Store_Ptr(name, ans))
            return;
        int loc = FuncSpTable->Query(name);
        if (loc == -1)
        {
            int nlen = name.length();
            string nname = name.substr(1, nlen - 1);
            ans += "\t la t1, " + nname + "\n";
            cout << "\t la t1, " << nname << endl;
            ans += "\tsw t0, 0(t1)\n";
            cout << "\tsw t0, 0(t1)\n";
        }
        else
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tsw t0, 0(t2)\n";
                cout << "\tsw t0, 0(t2)\n";
            }
            else
            {
                cout << "\tsw t0, " << loc << "(sp)" << endl;
                ans += "\tsw t0, " + to_string(loc) + "(sp)\n";
            }
        }
    }
    else if (val->kind.tag == KOOPA_RVT_FUNC_ARG_REF) // 4
    {
        cout << 33 << endl;
        // if (Store_Ptr(name, ans))
        //     return;
        int loc = FuncSpTable->Query(name);
        auto tp1 = val->kind.data.func_arg_ref.index;
        if (tp1 < 8)
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tsw a" + to_string(tp1) + ", 0(t2)\n";
                cout << "\tsw a" << tp1 << ", 0(t2)\n";
            }
            else
            {
                ans += "\tsw a" + to_string(tp1) + ", " + to_string(loc) + "(sp)\n";
                cout << "\tsw a" << tp1 << ", " << loc << "(sp)\n";
            }
        }
        //需要还原sp栈帧才能找到原始位置
        else
        {
            int par_loc = (tp1 - 8) * 4 + FuncSpTable->totsize;
            if (IntOverflow(par_loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                string loc_name = to_string(par_loc) + "(sp)";
                ans += "\tlw t0, " + loc_name + "\n";
                cout << "\tlw t0, " << loc_name << endl;
            }
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tsw t0, 0(t2)\n";
                cout << "\tsw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tsw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tsw t0, " << loc << "(sp)\n";
            }
        }
    }
    else if (val->kind.tag == KOOPA_RVT_CALL) // 15
    {
        cout << 34 << endl;
        char str[10];
        sprintf(str, "%p", &store.value->kind.data);
        string addr(str);
        int loc1 = FuncSpTable->Query(addr);
        if (IntOverflow(loc1, "t2", "sp", ans))
        {
            ans += "\tlw t0, 0(t2)\n";
            cout << "\tlw t0, 0(t2)\n";
        }
        else
        {
            ans += "\tlw t0, " + to_string(loc1) + "(sp)\n";
            cout << "\tlw t0, " << loc1 << "(sp)\n";
        }
        if (Store_Ptr(name, ans))
            return;
        int loc = FuncSpTable->Query(name);
        if (loc == -1) //全局变量
        {
            int l = name.length();
            string tname = name.substr(1, l - 1);
            ans += "\tla t1, " + tname + "\n";
            cout << "\tla t1, " << tname << endl;
            ans += "\tsw t0, 0(t1)\n";
            cout << "\tsw t0, 0(t1)\n";
        }
        else
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tsw a0, 0(t2)\n";
                cout << "\tsw a0, 0(t2)\n";
            }
            else
            {
                cout << "\tsw a0, " << loc << "(sp)" << endl;
                ans += "\tsw a0, " + to_string(loc) + "(sp)\n";
            }
        }
    }
    else if (val->kind.tag == KOOPA_RVT_LOAD) // 8
    {
        cout << 35 << endl;
        char str[10];
        sprintf(str, "%p", &store.value->kind.data.load);
        string tp(str);
        bool flag = false;
        int loc = FuncSpTable->Query(string(str));
        if (FuncSpTable->IsArr(tp))
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)\n";
            }
            ans += "\tlw t0, 0(t0)\n";
            cout << "\tlw t0, 0(t0)\n";
            flag = true;
        }
        else
        {
            if (IntOverflow(loc, "t2", "sp", ans))
            {
                ans += "\tlw t0, 0(t2)\n";
                cout << "\tlw t0, 0(t2)\n";
            }
            else
            {
                ans += "\tlw t0, " + to_string(loc) + "(sp)\n";
                cout << "\tlw t0, " << loc << "(sp)\n";
            }
        }
        if (loc != -1)
        {
            if (Store_Ptr(name, ans))
                return;
            int loc2 = FuncSpTable->Query(name);
            if (loc2 == -1)
            {
                int l = name.length();
                string tname = name.substr(1, l - 1);
                ans += "\tla t1, " + tname + "\n";
                cout << "\tla t1, " << tname << endl;
                ans += "\tsw t0, 0(t1)\n";
                cout << "\tsw t0, 0(t1)\n";
            }
            else
            {
                if (IntOverflow(loc2, "t2", "sp", ans))
                {
                    ans += "\tsw t0, 0(t2)\n";
                    cout << "\tsw t0, 0(t2)\n";
                }
                else
                {
                    ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
                    cout << "\tsw t0, " << loc2 << "(sp)\n";
                }
            }
        }
        else
        {
            int l = tp.length();
            string tname = tp.substr(1, l - 1);
            if (!flag)
            {
                ans += "\tla t0, " + tname + "\n";
                cout << "\tla t0, " << tname << "\n";
                ans += "\tlw t0, 0(t0)\n";
                cout << "\tlw t0, 0(t0)\n";
            }
            if (Store_Ptr(name, ans))
                return;
            int loc2 = FuncSpTable->Query(name);
            if (loc2 == -1)
            {
                int l = name.length();
                string tname = name.substr(1, l - 1);
                ans += "\tla t1, " + tname + "\n";
                cout << "\tla t1, " << tname << endl;
                ans += "\tsw t0, 0(t1)\n";
                cout << "\tsw t0, 0(t1)\n";
            }
            else
            {
                if (IntOverflow(loc2, "t2", "sp", ans))
                {
                    ans += "\tsw t0, 0(t2)\n";
                    cout << "\tsw t0, 0(t2)\n";
                }
                else
                {
                    ans += "\tsw t0, " + to_string(loc2) + "(sp)\n";
                    cout << "\tsw t0, " << loc2 << "(sp)\n";
                }
            }
        }
    }
    else
    {
        cout << "type:" << val->kind.tag << endl;
        assert(false);
    }
}

//处理alloc指令
void Visit(std::string alloc_name, std::string &ans)
{
    // cout<<"name: "<<alloc_name<<endl;
    auto tp = arr_size.find(alloc_name);
    if (tp == arr_size.end())
    {
        // cout<<111<<endl;
        int loc = FuncSpTable->cntsp;
        FuncSpTable->cntsp += 4;
        FuncSpTable->Insert(alloc_name, loc);
    }
    else
    {
        // cout<<222<<endl;
        int loc = FuncSpTable->cntsp;
        FuncSpTable->cntsp += tp->second.cap * 4;
        FuncSpTable->Insert(alloc_name, loc);
        FuncSpTable->InsertPtr(alloc_name, tp->second.v);
    }
}

//一个辅助函数，帮助生成二元运算式子的操作数
string Bi_op_src(koopa_raw_value_t &opr, std::string &ans, int order)
{
    string reg = "";
    auto tag = opr->kind.tag;
    char str[10];
    int loc;
    switch (tag)
    {
    case KOOPA_RVT_INTEGER:
        if (opr->kind.data.integer.value == 0)
        {
            reg = "x0";
        }
        else
        {
            reg = order == 0 ? "t0" : "t1";
            cout << "\tli " << reg << ", " << opr->kind.data.integer.value << endl;
            ans += "\tli " + reg + ", " + to_string(opr->kind.data.integer.value) + "\n";
        }
        break;

    case KOOPA_RVT_BINARY:
        sprintf(str, "%p", &opr->kind.data.binary);
        loc = FuncSpTable->Query(string(str));
        reg = order == 0 ? "t0" : "t1";
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw " + reg + ", 0(t2)\n";
            cout << "\tlw " << reg << ", 0(t2)\n";
        }
        else
        {
            ans += "\tlw " + reg + ", " + to_string(loc) + "(sp)\n";
            cout << "\tlw " << reg << ", " << loc << "(sp)\n";
        }
        break;

    case KOOPA_RVT_LOAD:
        sprintf(str, "%p", &opr->kind.data.load);
        loc = FuncSpTable->Query(string(str));
        reg = order == 0 ? "t0" : "t1";
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw " + reg + ", 0(t2)\n";
            cout << "\tlw " << reg << ", 0(t2)\n";
        }
        else
        {
            ans += "\tlw " + reg + ", " + to_string(loc) + "(sp)\n";
            cout << "\tlw " << reg << ", " << loc << "(sp)\n";
        }
        break;

    case KOOPA_RVT_CALL:
        sprintf(str, "%p", &opr->kind.data.call);
        cout << string(str) << endl;
        loc = FuncSpTable->Query(string(str));
        reg = order == 0 ? "t0" : "t1";
        if (IntOverflow(loc, "t2", "sp", ans))
        {
            ans += "\tlw " + reg + ", 0(t2)\n";
            cout << "\tlw " << reg << ", 0(t2)\n";
        }
        else
        {
            ans += "\tlw " + reg + ", " + to_string(loc) + "(sp)\n";
            cout << "\tlw " << reg << ", " << loc << "(sp)\n";
        }
        break;

    default:
        cout << "tag: " << tag << endl;
        assert(false);
    }
    return reg;
}

//处理所有的二元运算符（其实也就是所有的运算符了）
void Visit(const koopa_raw_binary_t &op, std::string &ans)
{
    // printf("123:%p\n",&op);
    koopa_raw_value_t lhs = op.lhs;
    koopa_raw_binary_op_t oper = op.op;
    koopa_raw_value_t rhs = op.rhs;
    string reg1, reg2;
    // cout<<"optype: "<<lhs->kind.tag<<" "<<rhs->kind.tag<<endl;
    reg1 = Bi_op_src(lhs, ans, 0);
    reg2 = Bi_op_src(rhs, ans, 1);
    switch (oper)
    {
    case KOOPA_RBO_NOT_EQ:
        ans += "\txor t0, " + reg1 + ", " + reg2 + "\n";
        ans += "\tsnez t0, t0\n";
        cout << "\txor t0, " << reg1 << ", " << reg2 << endl;
        cout << "\tsnez t0, t0" << endl;
        break;

    case KOOPA_RBO_EQ:
        ans += "\txor t0, " + reg1 + ", " + reg2 + "\n";
        ans += "\tseqz t0, t0\n";
        cout << "\txor t0, " << reg1 << ", " << reg2 << endl;
        cout << "\tseqz t0, t0" << endl;
        break;

    case KOOPA_RBO_GT:
        ans += "\tslt t0, " + reg2 + ", " + reg1 + "\n";
        cout << "\tslt t0, " << reg2 << ", " << reg1 << endl;
        break;

    case KOOPA_RBO_LT:
        ans += "\tslt t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tslt t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_GE:
        ans += "\tslt t0, " + reg1 + ", " + reg2 + "\n";
        ans += "\tseqz t0, t0\n";
        cout << "\tslt t0, " << reg1 << ", " << reg2 << endl;
        cout << "\tseqz t0, t0" << endl;
        break;

    case KOOPA_RBO_LE:
        ans += "\tsgt t0, " + reg1 + ", " + reg2 + "\n";
        ans += "\tseqz t0, t0\n";
        cout << "\tsgt t0, " << reg1 << ", " << reg2 << endl;
        cout << "\tseqz t0, t0" << endl;
        break;

    case KOOPA_RBO_ADD:
        ans += "\tadd t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tadd t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_SUB:
        ans += "\tsub t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tsub t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_MUL:
        ans += "\tmul t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tmul t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_DIV:
        ans += "\tdiv t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tdiv t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_MOD:
        ans += "\trem t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\trem t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_AND:
        ans += "\tand t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tand t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_OR:
        ans += "\tor t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tor t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_XOR:
        ans += "\txor t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\txor t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_SHL:
        ans += "\tsll t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tsll t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_SHR:
        ans += "\tsrl t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tsrl t0, " << reg1 << ", " << reg2 << endl;
        break;

    case KOOPA_RBO_SAR:
        ans += "\tsra t0, " + reg1 + ", " + reg2 + "\n";
        cout << "\tsra t0, " << reg1 << ", " << reg2 << endl;
        break;

    default:
        assert(false);
    }
    char str[10];
    sprintf(str, "%p", &op);
    string addr(str);
    int tploc = FuncSpTable->cntsp;
    if (IntOverflow(tploc, "t1", "sp", ans))
    {
        cout << "\tsw t0, 0(t1)\n";
        ans += "\tsw t0, 0(t1)\n";
    }
    else
    {
        cout << "\tsw t0, " << tploc << "(sp)" << endl;
        ans += "\tsw t0, " + to_string(tploc) + "(sp)\n";
    }
    FuncSpTable->cntsp += 4;
    FuncSpTable->Insert(addr, tploc);
}

//处理ret
void Visit(const koopa_raw_return_t &ret, std::string &ans)
{
    if (FuncSpTable->is_void == true)
    {
        if (exist_call)
        {
            int offset = FuncSpTable->totsize - 4;
            if (IntOverflow(offset, "t0", "sp", ans))
            {
                cout << "\tlw ra, 0(t0)\n";
                ans += "\tlw ra, 0(t0)\n";
            }
            else
            {
                cout << "\tlw ra, " << offset << "(sp)\n";
                ans += "\tlw ra, " + to_string(offset) + "(sp)\n";
            }
        }
        if (FuncSpTable->totsize < 2048)
        {
            cout << "\taddi sp, sp, " << FuncSpTable->totsize << "\n";
            ans += "\taddi sp, sp, " + to_string(FuncSpTable->totsize) + "\n";
        }
        else
        {
            cout << "\tli t0, " << FuncSpTable->totsize << "\n";
            ans += "\tli t0, " + to_string(FuncSpTable->totsize) + "\n";
            cout << "\tadd sp, sp, t0\n";
            ans += "\tadd sp, sp, t0\n";
        }
        cout << "\tret\n";
        ans += "\tret\n";
        return;
    }
    koopa_raw_value_t ret_value = ret.value;
    // cout<<"type_ret: "<<ret_value->kind.tag<<endl;
    if (ret_value->kind.tag == KOOPA_RVT_INTEGER)
    {
        ans += "\tli a0, " + to_string(ret_value->kind.data.integer.value) + "\n";
        cout << "\tli a0, " << ret_value->kind.data.integer.value << "\n";
    }
    else if (ret_value->kind.tag == KOOPA_RVT_BINARY)
    {
        cout << 41 << endl;
        char str[10];
        sprintf(str, "%p", &ret_value->kind.data.binary);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        if (IntOverflow(loc, "t0", "sp", ans))
        {
            cout << "\tlw a0, 0(t0)\n";
            ans += "\tlw a0, 0(t0)\n";
        }
        else
        {
            cout << "\tlw a0, " << loc << "(sp)" << endl;
            ans += "\tlw a0, " + to_string(loc) + "(sp)\n";
        }
    }
    else if (ret_value->kind.tag == KOOPA_RVT_STORE)
    {
        cout << 42 << endl;
        char str[10];
        sprintf(str, "%p", &ret_value->kind.data.store);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        if (IntOverflow(loc, "t0", "sp", ans))
        {
            cout << "\tlw a0, 0(t0)\n";
            ans += "\tlw a0, 0(t0)\n";
        }
        else
        {
            cout << "\tlw a0, " << loc << "(sp)" << endl;
            ans += "\tlw a0, " + to_string(loc) + "(sp)\n";
        }
    }
    else if (ret_value->kind.tag == KOOPA_RVT_ALLOC)
    {
        cout << 43 << endl;
        string name = ret_value->name;
        int loc = FuncSpTable->Query(name);
        if (IntOverflow(loc, "t0", "sp", ans))
        {
            cout << "\tlw a0, 0(t0)\n";
            ans += "\tlw a0, 0(t0)\n";
        }
        else
        {
            cout << "\tlw a0, " << loc << "(sp)" << endl;
            ans += "\tlw a0, " + to_string(loc) + "(sp)\n";
        }
    }
    else if (ret_value->kind.tag == KOOPA_RVT_LOAD)
    {
        cout << 44 << endl;
        char str[10];
        sprintf(str, "%p", &ret.value->kind.data.load);
        int loc = FuncSpTable->Query(string(str));
        if (IntOverflow(loc, "t0", "sp", ans))
        {
            cout << "\tlw a0, 0(t0)\n";
            ans += "\tlw a0, 0(t0)\n";
        }
        else
        {
            cout << "\tlw a0, " << loc << "(sp)" << endl;
            ans += "\tlw a0, " + to_string(loc) + "(sp)\n";
        }
    }
    else
    {
        cout << "new type" << ret_value->kind.tag << endl;
        assert(false);
    }
    if (exist_call)
    {
        int offset = FuncSpTable->totsize - 4;
        if (IntOverflow(offset, "t0", "sp", ans))
        {
            cout << "\tlw ra, 0(t0)\n";
            ans += "\tlw ra, 0(t0)\n";
        }
        else
        {
            cout << "\tlw ra, " << offset << "(sp)\n";
            ans += "\tlw ra, " + to_string(offset) + "(sp)\n";
        }
    }
    if (FuncSpTable->totsize < 2048)
    {
        cout << "\taddi sp, sp, " << FuncSpTable->totsize << "\n";
        ans += "\taddi sp, sp, " + to_string(FuncSpTable->totsize) + "\n";
    }
    else
    {
        cout << "\tli t0, " << FuncSpTable->totsize << "\n";
        ans += "\tli t0, " + to_string(FuncSpTable->totsize) + "\n";
        cout << "\tadd sp, sp, t0\n";
        ans += "\tadd sp, sp, t0\n";
    }
    cout << "\tret\n";
    ans += "\tret\n";
}

bool IntOverflow(int offset, std::string dest_reg, std::string bas_reg, std::string &ans)
{
    if (offset <= 2047 && offset >= -2048)
        return false;
    ans += "\tli " + dest_reg + ", " + to_string(offset) + "\n";
    cout << "\tli " << dest_reg << ", " << offset << endl;
    ans += "\tadd " + dest_reg + ", " + dest_reg + ", " + bas_reg + "\n";
    cout << "\tadd " << dest_reg << ", " << dest_reg << ", " << bas_reg << endl;
    return true;
}

//处理普通的立即数
void Visit(const koopa_raw_integer_t &integer, std::string &ans)
{
    int32_t int_val = integer.value;
    cout << "\tli a0, " << int_val << "\n";
    ans += "\tli a0, " + to_string(int_val) + "\n";
}
