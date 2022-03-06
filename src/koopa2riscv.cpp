#include<iostream>
#include<string>
#include<memory>
#include<cstdio>
#include<sstream>
#include<unordered_map>
#include<cassert>
#include<typeinfo>
#include"koopa.h"
#include"def.hpp"

using namespace std;

string reg_name[15] = {"t0","t1","t2","t3","t4","t5","t6","a0","a1","a2","a3","a4","a5","a6","a7"};
int reg_count = 0;  //从零开始的寄存器生活（bushi）
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

void Visit(const koopa_raw_function_t &func, std::string &ans)
{
    const char* f_name = func->name;
    string s =(f_name);
    int s_size = s.length();
    cout<<s.substr(1,s_size-1)<<":\n";
    ans+=s.substr(1,s_size-1)+":\n";
    auto params = func->params;
    Visit(func->bbs,ans);

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
    //cout<<"kind: "<<kind.tag<<endl;;
    switch(kind.tag)
    {
    case KOOPA_RVT_RETURN:
        Visit(kind.data.ret,ans);
        break;
    case KOOPA_RVT_INTEGER:
        Visit(kind.data.integer,ans);
        break;
    case KOOPA_RVT_BINARY:
        Visit(kind.data.binary,ans);
        break;
    default:
        assert(false);
    }
}

void Visit(const koopa_raw_binary_t &op, std::string &ans)
{
    koopa_raw_value_t lhs = op.lhs;
    koopa_raw_binary_op_t oper = op.op;
    koopa_raw_value_t rhs = op.rhs;
    //cout<<lhs->kind.tag<<" "<<oper<<" "<<rhs->kind.tag<<endl;
    //cout<<typeid(lhs).name()<<" "<<typeid(rhs).name()<<" "<<typeid(op).name()<<endl;
    
    string reg1, reg2;
    if(lhs->kind.tag==KOOPA_RVT_INTEGER && rhs->kind.tag==KOOPA_RVT_INTEGER)
    {
        if(lhs->kind.data.integer.value==0)
        {
            reg1 = "x0";
        }
        else
        {
            reg1 = reg_name[reg_count];
            ans+="\tli "+reg_name[reg_count]+", "+to_string(lhs->kind.data.integer.value)+"\n";
        }
        if(rhs->kind.data.integer.value==0)
        {
            reg2 = "x0";
        } 
        else
        {
            reg2 = reg_name[reg_count+1];
            ans+="\tli "+reg_name[reg_count+1]+", "+to_string(rhs->kind.data.integer.value)+"\n";
        }
        
    }
    else if(lhs->kind.tag==KOOPA_RVT_INTEGER && rhs->kind.tag==KOOPA_RVT_BINARY)
    {
        if(lhs->kind.data.integer.value==0)
        {
            reg1 = "x0";
        }
        else
        {
            ans+="\tli "+reg_name[reg_count]+", "+to_string(lhs->kind.data.integer.value)+"\n";
            reg1 = reg_name[reg_count];
        }
        reg2 = reg_name[reg_count-1];
    }
    else if(lhs->kind.tag==KOOPA_RVT_BINARY && rhs->kind.tag==KOOPA_RVT_INTEGER)
    {
        if(rhs->kind.data.integer.value==0)
        {
            reg2 = "x0";
        }
        else
        {
            ans+="\tli "+reg_name[reg_count]+", "+to_string(rhs->kind.data.integer.value)+"\n";
            reg2 = reg_name[reg_count];
        }
        reg1 = reg_name[reg_count-1];
    }
    else    //buggy
    {
        reg1 = reg_name[reg_count-2];
        reg2 = reg_name[reg_count-1];
    }
    switch(oper)
    {
        case KOOPA_RBO_NOT_EQ:
        ans+="\txor "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        ans+="\tsnez "+reg_name[reg_count]+", "+reg_name[reg_count]+"\n";
        break;
        
        case KOOPA_RBO_EQ:
        ans+="\txor "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        ans+="\tseqz "+reg_name[reg_count]+", "+reg_name[reg_count]+"\n";
        break;

        case KOOPA_RBO_GT:
        ans+="\tslt "+reg_name[reg_count]+", "+reg2+", "+reg1+"\n";
        break;

        case KOOPA_RBO_LT:
        ans+="\tslt "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_GE:
        ans+="\tslt "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        ans+="\tseqz "+reg_name[reg_count]+", "+reg_name[reg_count]+"\n";
        break;

        case KOOPA_RBO_LE:
        ans+="\tsgt "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        ans+="\tseqz "+reg_name[reg_count]+", "+reg_name[reg_count]+"\n";
        break;
        
        case KOOPA_RBO_ADD:
        ans+="\tadd "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_SUB:
        ans+="\tsub "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n"; 
        break;

        case KOOPA_RBO_MUL:
        ans+="\tmul "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_DIV:
        ans+="\tdiv "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_MOD:
        ans+="\trem "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_AND:
        ans+="\tand "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_OR:
        ans+="\tor "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_XOR:
        ans+="\txor "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_SHL:
        ans+="\tsll "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_SHR:
        ans+="\tsrl "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        case KOOPA_RBO_SAR:
        ans+="\tsra "+reg_name[reg_count]+", "+reg1+", "+reg2+"\n";
        break;

        default:
        assert(false);
    }
    reg_count++;
}


void Visit(const koopa_raw_return_t &ret, std::string &ans)
{
    koopa_raw_value_t ret_value = ret.value;
    cout<<ret_value->kind.tag<<endl;
    if(ret_value->kind.tag==KOOPA_RVT_INTEGER)
    {
        ans+="\tli a0, "+to_string(ret_value->kind.data.integer.value)+"\n";
        cout<<"\tli a0, "<<ret_value->kind.data.integer.value<<"\n";
    }
    if(ret_value->kind.tag==KOOPA_RVT_BINARY)
    {
        ans+="\tmv a0, "+reg_name[reg_count-1]+"\n";
        cout<<"\tmv a0, "<<reg_name[reg_count-1]<<endl;
    }
    cout<<"\tret\n";
    ans+="\tret\n";
}

void Visit(const koopa_raw_integer_t &integer, std::string &ans)
{
    int32_t int_val = integer.value;
    cout<<"\tli a0, "<<int_val<<"\n";
    ans+="\tli a0, "+to_string(int_val)+"\n";
}
