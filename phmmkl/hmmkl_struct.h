#ifndef __HMMKL_STRUCT_H__
#define __HMMKL_STRUCT_H__

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
using namespace std;
//////////////////////////////////////////////////////////////////////////
// dataset

struct FidxValue
{
	int fidx_;
	double value_;

	FidxValue(int fidx, double value):fidx_(fidx),value_(value){}
	FidxValue():fidx_(0),value_(0.0){}
};

struct Token
{
	int y_;
	FidxValue * fvec_;
	int fvec_size_;
};

struct Sentence
{
	Token * sent_;
	int token_size_;
};

struct Document
{
	Sentence * doc_;
	int sent_size_;
	int nclas_;
	int nfeat_;
};

//////////////////////////////////////////////////////////////////////////
// group structure
struct Group
{
	int *group_;
	int ele_size_;
};

struct GroupSet
{
	Group * group_set_;
	int num_groups_;
	int nfeat_;
};


// header information includes the group set information
void read_train_doc(const char * train_file, GroupSet & gset, Document & train_doc);
void read_test_doc(const char * test_file, Document & test_doc);

void clear(GroupSet &group_set);
void clear(Document &doc);

void print_struct(int sidx,Document & doc);

#endif