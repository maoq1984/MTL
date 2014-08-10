#ifndef __FEATUREEXTRACTOR_H__
#define __FEATUREEXTRACTOR_H__

#include <hash_map>
#include <vector>
#include <string>
#include <iostream>
#include "SFileFormat.h"
#include "TemplateSet.h"

using namespace std;
using namespace stdext;

class KeyFreq
{
public:
	string key_;
	int freq_;

	KeyFreq(string key,int freq):key_(key),freq_(freq){}
	KeyFreq(string key):key_(key),freq_(1){}
	~KeyFreq(){}
};


class BiDirectionMap
{
public:
	BiDirectionMap(){}
	~BiDirectionMap(){}

	int add(string key,int freq);
	string find_key(int idx){return idx2key_[idx].key_;}
	bool find(string key) 
	{
		if(key2idx_.find(key) != key2idx_.end()) 
			return true; 
		else 
			return false;
	}
	int find_idx(string key){return key2idx_[key];}

	void push_back(BiDirectionMap & new_one);

	int size(){return idx2key_.size();}

	KeyFreq get(int i) {return idx2key_[i];}


private:
	hash_map<string,int> key2idx_;
	vector<KeyFreq> idx2key_;
};


static const char *BOS[4] = { "_B-1", "_B-2", "_B-3", "_B-4"};
static const char *EOS[4] = { "_B+1", "_B+2", "_B+3", "_B+4"};

class FeatureExtractor
{
public:
	FeatureExtractor(TemplateSet *template_set,SFileFormat *data)
	{
		template_set_ = template_set;
		data_ = data;
	}

	string get_index(char *&p, size_t pos, vector<vector<string> > & sent);
	bool apply_template(string &os, char *p, size_t pos, vector<vector<string> > &sent);

	void build_label_map();
	void extract_features();

protected:
	TemplateSet *template_set_;
	SFileFormat *data_;

	vector<BiDirectionMap> featuremap_;
	vector<vector<vector<int> > > train_features_;
	vector<vector<vector<int> > > test_features_;

	BiDirectionMap labelmap_;
	vector<vector<int> > train_labels_;
	vector<vector<int> > test_labels_;
};

class HMSVM_Extractor : public FeatureExtractor
{
public:
	HMSVM_Extractor(TemplateSet *template_set,SFileFormat *data):FeatureExtractor(template_set,data)
	{
		build_label_map();
		extract_features();
	}

	void combine();
	void save(const char * train_file, const char * test_file, const char * map_file)
	{
		save_train_file(train_file);
		save_test_file(test_file);
		save_map(map_file);
	}

	void save_train_file(const char * train_file);
	void save_test_file(const char * test_file);
	void save_map(const char * map_file);

protected:
	vector<int> start_pos_;
	vector<int> group_size;
	BiDirectionMap combinedmap_;
	vector<vector<vector<int> > > combined_train_features_;
	vector<vector<vector<int> > > combined_test_features_;
};



#endif