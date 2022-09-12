#include<string>
#include<stack>
#include"koopa.h"
std::string txt2mmry(std::string);
void Visit(const koopa_raw_program_t &program, std::string &ans);

void Visit(const koopa_raw_slice_t &slice, std::string &ans);

void Visit(const koopa_raw_function_t &func, std::string &ans);

void Visit(const koopa_raw_basic_block_t &bb, std::string &ans);

void Visit(const koopa_raw_value_t &value, std::string &ans);

void Visit(const koopa_raw_return_t &ret, std::string &ans);

void Visit(const koopa_raw_integer_t &integer, std::string &ans);

void Visit(const koopa_raw_binary_t &op, std::string &ans);

void Visit(const koopa_raw_load_t &load, std::string &ans);

void Visit(const koopa_raw_value_t &value, const koopa_raw_store_t &store, std::string &ans);

void Visit(std::string alloc_name, std::string &ans);

void Visit(const koopa_raw_jump_t &jump, std::string &ans);

void Visit(const koopa_raw_branch_t &br, std::string &ans);

void Visit(const koopa_raw_call_t &call, std::string &ans);

void Visit(const koopa_raw_global_alloc_t &g_alloc, std::string alloc_name,std::string &ans);

void Visit(std::string ptrname, const koopa_raw_get_elem_ptr_t &getelemptr, std::string &ans);

void Visit(std::string ptrname, const koopa_raw_get_ptr_t &getptr, std::string &ans);

void GenOffset(int a, int b, std::string& ans);

int Pass1(const koopa_raw_function_t &func);

bool Store_Ptr(std::string name, std::string &ans);

void GenAggr(const koopa_raw_value_kind_t &g,std::string &ans);

bool IntOverflow(int offset, std::string dest_reg, std::string bas_reg,std::string &ans);