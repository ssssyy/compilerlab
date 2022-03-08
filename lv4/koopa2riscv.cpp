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
#include "localVarTab.hpp"

using namespace std;

string reg_name[15] = {"t0","t1","t2","t3","t4","t5","t6","a0","a1","a2","a3","a4","a5","a6","a7"};
int reg_count = 0;  //从零开始的寄存器生活（bushi）
LocalVarTable* FuncSpTable;
//unordered_map<string,string> lval2reg;  //维护表达式左值与所在寄存器的一个关系
void Visit(const koopa_raw_program_t &program, std::string &ans)
{
    Visit(program.values,ans);
    cout<<"\t.text\n";
    ans+="\t.text\n";
    Visit(program.funcs,ans);
}



void Visit(const koopa_raw_slice_t &slice, std::string &ans)
{
    //函数名 globl f1, f2部分
    if(slice.kind==KOOPA_RSIK_FUNCTION)
    {
        cout<<"\t.globl ";
        ans+="\t.globl ";
        for(size_t i = 0; i < slice.len; i++)
        {
            auto ptr = slice.buffer[i];
            const char* f_name = reinterpret_cast<koopa_raw_function_t>(ptr)->name;
            string ts(f_name);
            ts.erase(ts.begin());
            cout<<ts<<" ";
            ans+=ts+" ";
        }
        cout<<endl;
        ans+="\n";
    }
    for(size_t i = 0; i < slice.len; i++)
    {
        auto ptr = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr),ans);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),ans);
            break;
        case KOOPA_RSIK_VALUE:
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr),ans);
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
    koopa_raw_slice_t slice = func->bbs;
    int len =slice.len;
    //cout<<"Len: "<<len<<endl;
    for(size_t i = 0; i < len; i++)
    {
        auto ptr = slice.buffer[i];
        //cout<<slice.kind<<endl;
        if(slice.kind==KOOPA_RSIK_BASIC_BLOCK)
        {
            auto bb = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
            auto insts = bb->insts;
            int slen = insts.len;
            for(size_t j = 0; j < slen; j++)
            {
                auto ptr2 = insts.buffer[j];
                if(insts.kind==KOOPA_RSIK_VALUE)
                {
                    auto ptr3 = reinterpret_cast<koopa_raw_value_t>(ptr2);
                    auto ukind = ptr3->kind.tag;
                    //以后扩展更多指令
                    if(ukind!=KOOPA_RVT_RETURN&&ukind!=KOOPA_RVT_STORE)
                    {
                        ++tot;
                    }
                }
            }          
        }
    }
    //cout<<"tot "<<tot<<endl;
    return tot;
}



void Visit(const koopa_raw_function_t &func, std::string &ans)
{
    int sp_space = Pass1(func)*4;
    sp_space = sp_space%16?(sp_space/16+1)*16:sp_space;
    //cout<<"Size: "<<sp_space<<endl;
    const char* f_name = func->name;
    string s =(f_name);
    int s_size = s.length();
    cout<<s.substr(1,s_size-1)<<":\n";
    ans+=s.substr(1,s_size-1)+":\n";
    //假设不超过2048字节
    cout<<"\taddi sp, sp, -"<<sp_space<<endl;
    ans+="\taddi sp, sp, -"+to_string(sp_space)+"\n";
    LocalVarTable *sp_table = new LocalVarTable(s,sp_space);
    FuncSpTable = sp_table;
    auto params = func->params;
    Visit(func->bbs,ans);
    delete sp_table;
}

void Visit(const koopa_raw_basic_block_t &bb, std::string &ans)
{
    const char* b_name = bb->name;
    auto params = bb->params;
    Visit(bb->insts,ans);
}


void Visit(const koopa_raw_value_t &value, std::string &ans)
{
    const auto &kind = value->kind;
    //cout<<"kind: "<<kind.tag<<endl;
    switch(kind.tag)
    {
    case KOOPA_RVT_RETURN:  //16
        Visit(kind.data.ret,ans);
        break;
    case KOOPA_RVT_INTEGER: //0
        Visit(kind.data.integer,ans);
        break;
    case KOOPA_RVT_BINARY:  //12
        Visit(kind.data.binary,ans);
        break;
    case KOOPA_RVT_ALLOC:   //6
        Visit(value->name,ans);
        break;
    case KOOPA_RVT_LOAD:    //8
        Visit(kind.data.load,ans);
        break;
    case KOOPA_RVT_STORE:   //9
        Visit(value, kind.data.store,ans);
        break;
    default:
        assert(false);
    }
}

void Visit(const koopa_raw_load_t &load, std::string &ans)
{
    auto tag = load.src->kind.tag;  //要加载的源的类型，alloc/binary
    if(tag==KOOPA_RVT_ALLOC)
    {
        string name = load.src->name;
        int loc1 = FuncSpTable->Query(name);
        cout<<"\tlw t0, "<<loc1<<"(sp)"<<endl;
        ans+="\tlw t0, "+to_string(loc1)+"(sp)\n";
        int loc2 = FuncSpTable->cntsp;
        FuncSpTable->cntsp+=4;
        cout<<"\tsw t0, "<<loc2<<"(sp)"<<endl;
        ans+="\tsw t0, "+to_string(loc2)+"(sp)\n";
        char str[10];
        sprintf(str,"%p",&load);
        string addr(str);
        FuncSpTable->Insert(addr,loc2);
    }
    
    
}

//处理store指令
void Visit(const koopa_raw_value_t &value, const koopa_raw_store_t &store, std::string &ans)
{
    koopa_raw_value_t dest = store.dest;
    koopa_raw_value_t val = store.value;
    string name = dest->name;
    //cout<<"type:"<<val->kind.tag<<endl;
    if(val->kind.tag==KOOPA_RVT_BINARY)
    {
        char str[10];
        sprintf(str,"%p",&val->kind.data.binary);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        cout<<"\tlw t0, "<<loc<<"(sp)"<<endl;
        ans+="\tlw t0, "+to_string(loc)+"(sp)\n";
    }
    else if(val->kind.tag==KOOPA_RVT_INTEGER)
    {
        int val1 = val->kind.data.integer.value;
        cout<<"\tli t0, "<<val1<<endl;
        ans+="\tli t0, "+to_string(val1)+"\n";
    }
    int loc = FuncSpTable->Query(name);
    cout<<"\tsw t0, "<<loc<<"(sp)"<<endl;
    ans+="\tsw t0, "+to_string(loc)+"(sp)\n";
    FuncSpTable->cntsp+=4;

}

//处理alloc指令
void Visit(std::string alloc_name, std::string &ans)
{
    int loc = FuncSpTable->cntsp;
    FuncSpTable->cntsp+=4;
    FuncSpTable->Insert(alloc_name,loc);
    //FuncSpTable->Print();
}

string Bi_op_src(koopa_raw_value_t &opr, std::string &ans,int order)
{
    string reg = "";
    auto tag = opr->kind.tag;
    char str[10];
    int loc;
    switch(tag)
    {
        case KOOPA_RVT_INTEGER:
        if(opr->kind.data.integer.value==0)
        {
            reg = "x0";

        }
        else
        {
            reg = order==0?"t0":"t1";
            cout<<"\tli "<<reg<<", "<<opr->kind.data.integer.value<<endl;
            ans+="\tli "+reg+", "+to_string(opr->kind.data.integer.value)+"\n";
        }
        break;

        case KOOPA_RVT_BINARY:
        sprintf(str,"%p",&opr->kind.data.binary);
        loc = FuncSpTable->Query(string(str));
        reg = order==0?"t0":"t1";
        ans+="\tlw "+reg+", "+to_string(loc)+"(sp)\n";
        cout<<"\tlw "<<reg<<", "<<loc<<"(sp)\n";
        break;

        case KOOPA_RVT_LOAD:
        sprintf(str,"%p",&opr->kind.data.load);
        loc = FuncSpTable->Query(string(str));
        reg = order==0?"t0":"t1";
        ans+="\tlw "+reg+", "+to_string(loc)+"(sp)\n";
        cout<<"\tlw "<<reg<<", "<<loc<<"(sp)\n";
        break;

        default:
        cout<<tag<<endl;
        assert(false);
    }
    return reg;
}



//处理所有的二元运算符（其实也就是所有的运算符了）
void Visit(const koopa_raw_binary_t &op, std::string &ans)
{
    koopa_raw_value_t lhs = op.lhs;
    koopa_raw_binary_op_t oper = op.op;
    koopa_raw_value_t rhs = op.rhs;
    string reg1, reg2;
    cout<<"optype: "<<lhs->kind.tag<<" "<<rhs->kind.tag<<endl;
    reg1 = Bi_op_src(lhs,ans,0);
    reg2 = Bi_op_src(rhs,ans,1);
    switch(oper)
    {
        case KOOPA_RBO_NOT_EQ:
        ans+="\txor t0, "+reg1+", "+reg2+"\n";
        ans+="\tsnez t0, t0\n";
        cout<<"\txor t0, "<<reg1<<", "<<reg2<<endl;
        cout<<"\tsnez t0, t0"<<endl;
        break;
        
        case KOOPA_RBO_EQ:
        ans+="\txor t0, "+reg1+", "+reg2+"\n";
        ans+="\tseqz t0, t0\n";
        cout<<"\txor t0, "<<reg1<<", "<<reg2<<endl;
        cout<<"\tseqz t0, t0"<<endl;
        break;

        case KOOPA_RBO_GT:
        ans+="\tslt t0, "+reg2+", "+reg1+"\n";
        cout<<"\tslt t0, "<<reg2<<", "<<reg1<<endl;
        break;

        case KOOPA_RBO_LT:
        ans+="\tslt t0, "+reg1+", "+reg2+"\n";
        cout<<"\tslt t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_GE:
        ans+="\tslt t0, "+reg1+", "+reg2+"\n";
        ans+="\tseqz t0, t0\n";
        cout<<"\tslt t0, "<<reg1<<", "<<reg2<<endl;
        cout<<"\tseqz t0, t0"<<endl;
        break;

        case KOOPA_RBO_LE:
        ans+="\tsgt t0, "+reg1+", "+reg2+"\n";
        ans+="\tseqz t0, t0\n";
        cout<<"\tsgt t0, "<<reg1<<", "<<reg2<<endl;
        cout<<"\tseqz t0, t0"<<endl;
        break;
        
        case KOOPA_RBO_ADD:
        ans+="\tadd t0, "+reg1+", "+reg2+"\n";
        cout<<"\tadd t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_SUB:
        ans+="\tsub t0, "+reg1+", "+reg2+"\n";
        cout<<"\tsub t0, "<<reg1<<", "<<reg2<<endl; 
        break;

        case KOOPA_RBO_MUL:
        ans+="\tmul t0, "+reg1+", "+reg2+"\n";
        cout<<"\tmul t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_DIV:
        ans+="\tdiv t0, "+reg1+", "+reg2+"\n";
        cout<<"\tdiv t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_MOD:
        ans+="\trem t0, "+reg1+", "+reg2+"\n";
        cout<<"\trem t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_AND:
        ans+="\tand t0, "+reg1+", "+reg2+"\n";
        cout<<"\tand t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_OR:
        ans+="\tor t0, "+reg1+", "+reg2+"\n";
        cout<<"\tor t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_XOR:
        ans+="\txor t0, "+reg1+", "+reg2+"\n";
        cout<<"\txor t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_SHL:
        ans+="\tsll t0, "+reg1+", "+reg2+"\n";
        cout<<"\tsll t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_SHR:
        ans+="\tsrl t0, "+reg1+", "+reg2+"\n";
        cout<<"\tsrl t0, "<<reg1<<", "<<reg2<<endl;
        break;

        case KOOPA_RBO_SAR:
        ans+="\tsra t0, "+reg1+", "+reg2+"\n";
        cout<<"\tsra t0, "<<reg1<<", "<<reg2<<endl;
        break;

        default:
        assert(false);
    }
    char str[10];
    sprintf(str,"%p",&op);
    string addr(str);
    int tploc = FuncSpTable->cntsp;
    cout<<"\tsw t0, "<<tploc<<"(sp)"<<endl;
    ans+="\tsw t0, "+to_string(tploc)+"(sp)\n";
    FuncSpTable->cntsp+=4;
    FuncSpTable->Insert(addr,tploc);
}

//处理ret
void Visit(const koopa_raw_return_t &ret, std::string &ans)
{
    koopa_raw_value_t ret_value = ret.value;
    //cout<<"type_ret: "<<ret_value->kind.tag<<endl;
    if(ret_value->kind.tag==KOOPA_RVT_INTEGER)
    {
        ans+="\tli a0, "+to_string(ret_value->kind.data.integer.value)+"\n";
        cout<<"\tli a0, "<<ret_value->kind.data.integer.value<<"\n";
    }
    else if(ret_value->kind.tag==KOOPA_RVT_BINARY)
    {
        char str[10];
        sprintf(str,"%p",&ret_value->kind.data.binary);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        cout<<"\tlw a0, "<<loc<<"(sp)"<<endl;
        ans+="\tlw a0, "+to_string(loc)+"(sp)\n";
    }
    else if(ret_value->kind.tag==KOOPA_RVT_STORE)
    {
        char str[10];
        sprintf(str,"%p",&ret_value->kind.data.store);
        string addr(str);
        int loc = FuncSpTable->Query(addr);
        cout<<"\tlw a0, "<<loc<<"(sp)"<<endl;
        ans+="\tlw a0, "+to_string(loc)+"(sp)\n";
    }
    else if(ret_value->kind.tag==KOOPA_RVT_ALLOC)
    {
        string name = ret_value->name;
        int loc = FuncSpTable->Query(name);
        cout<<"\tlw a0, "<<loc<<"(sp)"<<endl;
        ans+="\tlw a0, "+to_string(loc)+"(sp)\n";
    }
    else if(ret_value->kind.tag==KOOPA_RVT_LOAD)
    {
        auto t = ret_value->kind.data.load.src->name;
        //cout<<t<<endl;
        int loc = FuncSpTable->Query(t);
        cout<<"\tlw a0, "<<loc<<"(sp)"<<endl;
        ans+="\tlw a0, "+to_string(loc)+"(sp)\n";
    }
    cout<<"\taddi sp, sp, "<<FuncSpTable->totsize<<"\n";
    ans+="\taddi sp, sp, "+to_string(FuncSpTable->totsize)+"\n";
    cout<<"\tret\n";
    ans+="\tret\n";
}

//处理普通的立即数
void Visit(const koopa_raw_integer_t &integer, std::string &ans)
{
    int32_t int_val = integer.value;
    cout<<"\tli a0, "<<int_val<<"\n";
    ans+="\tli a0, "+to_string(int_val)+"\n";
}
