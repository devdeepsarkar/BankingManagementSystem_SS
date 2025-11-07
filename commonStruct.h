#ifndef COMMON_STRUCT_H
#define COMMON_STRUCT_H

#include <stdbool.h>

struct customer
{
	char name[128];
	int customer_id;
	int account_number;
	int balance;
	bool need_loan;
	int loan_amount;
	bool loan_approved;
	char password[128];
	bool is_online;
	bool is_active;
	int contact;
	char address[1024];
};

struct employee
{
	int emp_id;
	char username[128];
	char password[128];
	bool is_online;
	bool is_active;
};

struct loan
{
	int emp_id;
	int customer_id;
	int loan_amount;
	char status[64];
};

struct manager
{
	int emp_id;
	char username[128];
	char password[128];
	bool is_online;
};

struct admin
{
	int emp_id;
	char username[128];
	char password[128];
	bool is_online;
};

struct transact
{
	int user_id;
	char transaction[256];
	char date_time[128];
	int current_balance;
};

struct feedback
{
	int user_id;
	bool resolved;
	char feed_back[1024];
};

#endif