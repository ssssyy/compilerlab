#include <iostream>
#include <cstring>
#include <cstdio>
#include <memory>
#include <stack>
#include "def.hpp"

extern std::string name_table[5];
extern int tpname_count;    //对临时名字进行计数
class BaseAST{
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::string&) const = 0;  //输出AST
    virtual void dfs(std::string&, std::stack<std::string>&) const = 0;   //生成IR
};

class CompUnitAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump(std::string &ast) const override{
        std::cout<<"CompUnitAST { ";
        ast+="CompUnitAST { ";
        func_def->Dump(ast);
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string &ans, std::stack<std::string>& aststack) const override{
        func_def->dfs(ans,aststack);    //暂时不用输出
    }
};

class FuncDefAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump(std::string &ast) const override{
        std::cout<<"FuncDefAST { ";
        ast+="FuncDefAST { ";
        func_type->Dump(ast);
        std::cout<<", "<<ident<<", ";
        ast+=", "+ident+", ";
        block->Dump(ast);
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string &ans, std::stack<std::string>& aststack) const override
    {
        std::cout<<"fun ";
        ans+="fun ";
        std::cout<<"@"+ident+"(";
        ans+="@"+ident+"(";
        std::cout<<"): ";
        ans+="): ";
        func_type->dfs(ans,aststack);
        block->dfs(ans,aststack);
    }
};

class FuncTypeAST: public BaseAST{
public:
    void Dump(std::string &ast) const override{
        std::cout<<"FuncTypeAST { "<<"int"<<" }";
        ast+="FuncTypeAST { int }";
    }
    void dfs(std::string &ans, std::stack<std::string>& aststack) const override{
        std::cout<<"i32";
        ans+="i32";
    }
};

class BlockAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump(std::string &ast) const override{
        std::cout<<"BlockAST { ";
        ast+="BlockAST { ";
        stmt->Dump(ast);
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string &ans, std::stack<std::string>& aststack) const override{
        std::cout<<"{\n";
        ans+="{\n";
        std::cout<<"\%"<<name_table[0]<<":\n";
        ans+="\%"+name_table[0]+":\n";
        stmt->dfs(ans,aststack);
        genAST(ans,aststack);
        /*
        std::cout<<"\n\n";
        while(!aststack.empty())
        {
            std::cout<<aststack.top()<<" ";
            aststack.pop();
        }
        std::cout<<"\n\n";
        */
        std::cout<<"}\n";
        ans+="}\n";
    }
    //运算符则执行对应运算，括号忽略，其他的塞进辅助栈
    void genAST(std::string &ans, std::stack<std::string>& aststack) const 
    {
        std::stack<std::string> fuzhu;  //辅助栈，用来对后缀表达式求值
        while(!aststack.empty())
        {
            std::string val1,val2,val3;
            val1 = aststack.top();
            aststack.pop();
            if(val1=="(") continue;
            if(val1==")") continue;
            if(val1=="ret")
            {
                ans+="\tret "+fuzhu.top()+"\n";
                fuzhu.pop();
                continue;
            }
            if(val1=="+ 1")
            {
                continue;
            }
            if(val1=="- 1")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                ans+="\t"+tpname+" = sub 0, "+fuzhu.top()+"\n";
                fuzhu.pop();
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="! 1")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                ans+="\t"+tpname+" = eq "+fuzhu.top()+", 0\n";
                fuzhu.pop();
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="+")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = add "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="-")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = sub "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="*")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = mul "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="/")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = div "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="%")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = mod "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="<")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = lt "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1==">")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = gt "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="<=")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = le "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1==">=")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = ge "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="==")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = eq "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="!=")
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = ne "+val2+", "+val3+"\n";
                fuzhu.push(tpname);
                continue;
            }
            if(val1=="&&")  
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                std::string ttpname = "%"+std::to_string(tpname_count++);
                std::string tttpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = ne 0, "+val2+"\n";
                ans+="\t"+ttpname+" = ne 0, "+val3+"\n";
                ans+="\t"+tttpname+" = and "+tpname+", "+ttpname+"\n";
                fuzhu.push(tttpname);
                continue;
            }
            if(val1=="||")  
            {
                std::string tpname = "%"+std::to_string(tpname_count++);
                val2 = fuzhu.top();
                fuzhu.pop();
                val3 = fuzhu.top();
                fuzhu.pop();
                ans+="\t"+tpname+" = or "+val2+", "+val3+"\n";
                std::string ttpname = "%"+std::to_string(tpname_count++);
                ans+="\t"+ttpname+" = ne 0, "+tpname+"\n";
                fuzhu.push(ttpname);
                continue;
            }
            fuzhu.push(val1);    
        }
    }
};

class StmtAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump(std::string& ast) const override{
        std::cout<<"StmtAST { ";
        ast+="StmtAST { ";
        exp->Dump(ast);
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string &ans,std::stack<std::string>& aststack) const override{
        std::cout<<"\tret ";
        aststack.push("ret");
        exp->dfs(ans,aststack);
        std::cout<<"\n";
    }
};

class ExpAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> lor_exp;
    void Dump(std::string& ast) const override{
        std::cout<<"ExpAST { ";
        ast+="ExpAST { ";
        lor_exp->Dump(ast);
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string &ans,std::stack<std::string>& aststack) const override{
        lor_exp->dfs(ans,aststack);
    }
};

class UnaryExpAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_op;
    std::unique_ptr<BaseAST> unary_exp;
    void Dump(std::string& ast) const override{
        std::cout<<"UnaryExpAST { ";
        ast+="UnaryExpAST { ";
        if(primary_exp!=nullptr)    //生成了PrimaryExp单替换
        {
            primary_exp->Dump(ast);
            std::cout<<" }";
            ast+=" }";
        }
        else if(unary_op!=nullptr && unary_exp!=nullptr)
        {
            unary_op->Dump(ast);
            std::cout<<", ";
            ast+=", ";
            unary_exp->Dump(ast);
            std::cout<<" }";
            ast+=" }";
        }
    }
    void dfs(std::string &ans,std::stack<std::string>& aststack) const override{
        if(primary_exp!=nullptr)
        {
            primary_exp->dfs(ans,aststack);
        }
        else if(unary_op!=nullptr && unary_exp!=nullptr)
        {
            unary_op->dfs(ans,aststack);
            unary_exp->dfs(ans,aststack);
        }
    }
};

class PrimaryExpAST: public BaseAST{
public:
    int number;
    std::unique_ptr<BaseAST> exp;
    void Dump(std::string& ast) const override{
        std::cout<<"PrimaryExpAST { ";
        ast+="PrimaryExpAST { ";
        if(exp!=nullptr)
        {
            std::cout<<"(";
            ast+="(";
            exp->Dump(ast);
            std::cout<<")";
            ast+=")";
        }
        else
        {
            std::cout<<number;
            ast+=std::to_string(number);
        }
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(exp!=nullptr)
        {
            std::cout<<"(";
            aststack.push("(");
            exp->dfs(ans,aststack);
            std::cout<<")";
            aststack.push(")");
        }
        else
        {
            std::cout<<number;
            aststack.push(std::to_string(number));
        }
    }
};

class UnaryOpAST: public BaseAST{
public:
    std::string unary_op;  // + - !
    void Dump(std::string& ast) const override{
        std::cout<<"UnaryOpAST { ";
        ast+="UnaryOpAST { ";
        std::cout<<unary_op;
        ast+=unary_op;
        std::cout<<" }";
        ast+=" }";
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        std::cout<<unary_op<<" ";
        aststack.push(unary_op+" 1");
    }
};

class AddExpAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string add_op; //+ -, binary op
    void Dump(std::string& ast) const override{
        std::cout<<"AddExpAST { ";
        ast+="AddExpAST { ";
        if(add_exp!=nullptr)
        {
            add_exp->Dump(ast);
            std::cout<<" "<<add_op<<" ";
            ast+=" "+add_op+" ";
            mul_exp->Dump(ast);
        }
        else
        {
            mul_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(add_exp!=nullptr)
        {
            aststack.push(add_op);
            std::cout<<add_op<<" ";
            add_exp->dfs(ans,aststack);
            mul_exp->dfs(ans,aststack);
        }
        else
        {
            mul_exp->dfs(ans,aststack);
        }
    }
};

class MulExpAST: public BaseAST{
public:
    std::unique_ptr<BaseAST> unary_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string mul_op; //* / %, binary op
    void Dump(std::string& ast) const override{
        std::cout<<"MulExpAST { ";
        ast+="MulExpAST { ";
        if(mul_exp!=nullptr)
        {
            mul_exp->Dump(ast);
            std::cout<<" "<<mul_op<<" ";
            ast+=" "+mul_op+" ";
            unary_exp->Dump(ast);
        }
        else
        {
            unary_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(mul_exp!=nullptr)
        {
            aststack.push(mul_op);
            std::cout<<mul_op<<" ";
            mul_exp->dfs(ans,aststack);
            unary_exp->dfs(ans,aststack);
        }
        else
        {
            unary_exp->dfs(ans,aststack);
        }
    }
};

class LOrExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> lor_exp;
    void Dump(std::string& ast) const override{
        std::cout<<"LOrExpAST { ";
        ast+="LOrExpAST { ";
        if(lor_exp!=nullptr)
        {
            lor_exp->Dump(ast);
            std::cout<<" || ";
            ast+=" || ";
            land_exp->Dump(ast);
        }
        else
        {
            land_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(lor_exp!=nullptr)
        {
            aststack.push("||");
            std::cout<<"||"<<" ";
            lor_exp->dfs(ans,aststack);
            land_exp->dfs(ans,aststack);
        }
        else
        {
            land_exp->dfs(ans,aststack);
        }
    }
};

class LAndExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;
    void Dump(std::string& ast) const override{
        std::cout<<"LAndExpAST { ";
        ast+="LAndExpAST { ";
        if(land_exp!=nullptr)
        {
            land_exp->Dump(ast);
            std::cout<<" && ";
            ast+=" && ";
            eq_exp->Dump(ast);
        }
        else
        {
            eq_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(land_exp!=nullptr)
        {
            aststack.push("&&");
            std::cout<<"&&"<<" ";
            land_exp->dfs(ans,aststack);
            eq_exp->dfs(ans,aststack);
        }
        else
        {
            eq_exp->dfs(ans,aststack);
        }
    }
};

class EqExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> eq_exp;
    std::string eq_op;
    void Dump(std::string& ast) const override{
        std::cout<<"EqExpAST { ";
        ast+="EqExpAST { ";
        if(eq_exp!=nullptr)
        {
            eq_exp->Dump(ast);
            std::cout<<" "<<eq_op<<" ";
            ast+=" "+eq_op+" ";
            rel_exp->Dump(ast);
        }
        else
        {
            rel_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(eq_exp!=nullptr)
        {
            aststack.push(eq_op);
            std::cout<<eq_op<<" ";
            eq_exp->dfs(ans,aststack);
            rel_exp->dfs(ans,aststack);
        }
        else
        {
            rel_exp->dfs(ans,aststack);
        }
    }
};

class RelExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string rel_op;
    void Dump(std::string& ast) const override{
        std::cout<<"RelExpAST { ";
        ast+="RelExpAST { ";
        if(rel_exp!=nullptr)
        {
            rel_exp->Dump(ast);
            std::cout<<" "<<rel_op<<" ";
            ast+=" "+rel_op+" ";
            add_exp->Dump(ast);
        }
        else
        {
            add_exp->Dump(ast);
        }
    }
    void dfs(std::string& ans, std::stack<std::string>& aststack) const override{
        if(rel_exp!=nullptr)
        {
            aststack.push(rel_op);
            std::cout<<rel_op<<" ";
            rel_exp->dfs(ans,aststack);
            add_exp->dfs(ans,aststack);
        }
        else
        {
            add_exp->dfs(ans,aststack);
        }
    }
};

