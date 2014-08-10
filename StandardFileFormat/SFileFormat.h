#ifndef __SFILEFORMAT_H__
#define __SFILEFORMAT_H__

#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <iostream>
using namespace std;


class RandPerm
{
public:
	RandPerm(int size, int rand_size)
	{
		size_ = size;
		rand_size_ = rand_size;
	}

	vector<int> run_rand();
	vector<int> get(){return perm_;}

	~RandPerm(){}

private:
	int size_;
	int rand_size_;
	vector<int> perm_;
};

vector<string> gbstring(const char * str);
vector<string> uft8string(const char* pBuffer);

class SFileFormat
{
public:
	SFileFormat(){}
	~SFileFormat(){}

	void load(const char *train_file, const char *test_file);	
	void load(const char *file, double ratio);	

	void parse()
	{
		parse(train_sents,train_doc);
		parse(test_sents,test_doc);
	}

	virtual void parse(vector<string> sent, vector<vector<vector<string> > > &doc) = 0;

	void save(const char * save_train_file, const char *save_test_file)
	{
		save_file(save_train_file,train_doc);
		save_file(save_test_file,test_doc);
	}

	int get_column() { 	return column; }

	string get_train_label(int i, int j) {return train_doc[i][j][column];}

	int train_size() {return train_doc.size();}
	vector<vector<string> > get_train(int i) {return train_doc[i];}
	int test_size() {return test_doc.size();}
	vector<vector<string> > get_test(int i) {return test_doc[i];}

protected:
	void load_file(const char *file, vector<string> & sents);	
	void save_file(const char *file,vector<vector<vector<string> > > &doc);

protected:
	vector<vector<vector<string> > > train_doc;
	vector<vector<vector<string> > > test_doc;

	vector<string> train_sents;
	vector<string> test_sents;

	int column;
};



class Seg_SFileFormat : public SFileFormat
{
public:
	Seg_SFileFormat(){}
	~Seg_SFileFormat(){}

	void parse(vector<string> sent, vector<vector<vector<string> > > &doc);

};

class NER_CONLL_SFileFormat : public SFileFormat
{
public:
	NER_CONLL_SFileFormat(){}
	~NER_CONLL_SFileFormat(){}

	void parse(vector<string> sent, vector<vector<vector<string> > > &doc);
};


#endif