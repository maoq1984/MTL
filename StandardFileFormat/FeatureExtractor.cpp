#include "FeatureExtractor.h"

int BiDirectionMap::add(string key,int freq)
{
	int idx = -1;
	if(key2idx_.find(key) != key2idx_.end()) // find
	{
		idx = key2idx_[key];
		idx2key_[idx].freq_ += freq;
	}
	else
	{
		idx = idx2key_.size();
		KeyFreq kf(key,freq);
		idx2key_.push_back(kf);
		key2idx_[key] = idx;
	}
	return idx;
}

void BiDirectionMap::push_back(BiDirectionMap & new_one)
{
	for(int i = 0;i<new_one.size();i++)
	{
		int idx = idx2key_.size();
		string key = new_one.idx2key_[i].key_;
		idx2key_.push_back(new_one.idx2key_[i]);
		key2idx_[key] = idx;
	}
}

//////////////////////////////////////////////////////////////////////////
string FeatureExtractor::get_index(char *&p, size_t pos, vector<vector<string> > & sent)
{
	if (*p++ !='[') return 0;

	int col = 0;
	int row = 0;

	int neg = 1;
	if (*p++ == '-')
		neg = -1;
	else
		--p;

	for (; *p; ++p)
	{
		switch (*p)
		{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			row = 10 * row +(*p - '0');
			break;
		case ',':
			++p;
			goto NEXT1;
		default: return  0;
		}
	}

NEXT1:

	for (; *p; ++p)
	{
		switch (*p)
		{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			col = 10 * col +(*p - '0');
			break;
		case ']': goto NEXT2;
		default: return 0;
		}
	}

NEXT2:

	row *= neg;

	if (row < -4 || row > 4 || col < 0 || col >= data_->get_column())
		return 0;

	int idx = pos + row;
	if (idx < 0)
		return string(BOS[-idx-1]);

	int sent_size = sent.size();
	if (idx >= sent_size)
		return string(EOS[idx - sent.size()]);

	return sent[idx][col];
}

bool FeatureExtractor::apply_template(string &os, char *p, size_t pos, vector<vector<string> > &sent)
{
	os = "";
	string r="";

	for (; *p; p++) 
	{
		switch (*p) 
		{
		default:
			os += *p;
			break;
		case '%':
			switch (*++p) 
			{
			case 'x':
				++p;
				r = get_index(p, pos, sent);
				if (r.empty()) 
					return false;
				os.append(r);
				break;
			default:
				return false;
			}
			break;
		}
	}	
	return true;
}


void FeatureExtractor::build_label_map()
{
	/* for training label*/
	int column = data_->get_column();
	int train_sent_size = data_->train_size();
	for(int j=0;j<train_sent_size;j++)
	{
		vector<int> temp_train_sent;

		vector<vector<string> > sent = data_->get_train(j);
		int word_len = sent.size();
		for(int pos=0;pos<word_len;pos++)
		{
			string key = sent[pos][column];
			int idx = labelmap_.add(key,1);
			temp_train_sent.push_back(idx);
		}
		train_labels_.push_back(temp_train_sent);
	}

	/* for testing label*/
	int test_sent_size = data_->test_size();
	for(int j = 0;j<test_sent_size;j++)
	{
		vector<int> temp_test_sent;

		vector<vector<string> > sent = data_->get_test(j);
		int word_len = sent.size();
		for(int pos=0;pos<word_len;pos++)
		{
			string key = sent[pos][column];
			if(labelmap_.find(key))
			{
				int idx = labelmap_.find_idx(key);
				temp_test_sent.push_back(idx);
			}
			else // test label does not appeared in training label
			{
				temp_test_sent.push_back(1);
				cout<<"the label "<< key<<" appeared in testing data does not appear in training data"<<endl;
			}
		}
		test_labels_.push_back(temp_test_sent);
	}
}

void FeatureExtractor::extract_features()
{
	/*for training data*/
	cout<<"extract training data..."<<endl;

	int template_size = template_set_->size();
	for(int i = 0;i<template_size;i++)
	{
		cout<<"the "<<i<<"th template"<<endl;

		BiDirectionMap fmap;
		featuremap_.push_back(fmap);

		vector<vector<int> > temp_template;

		string one_template = template_set_->get(i);
		char * ptr = const_cast<char *>(one_template.c_str());

		int train_sent_size = data_->train_size();
		for(int j = 0;j<train_sent_size;j++)
		{

			if((j+1) % 100 == 0)
			{
				cout<<".";
				if((j+1) %5000 == 0)
					cout<<endl;
			}

			vector<int > temp_sent;

			vector<vector<string> > sent = data_->get_train(j);
			int word_len = sent.size();
			for(int pos=0;pos<word_len;pos++)
			{
				string one_feature;
				apply_template(one_feature,ptr,pos,sent);

				int idx = featuremap_[i].add(one_feature,1);

				temp_sent.push_back(idx);
			}
			temp_template.push_back(temp_sent);
		}
		train_features_.push_back(temp_template);
	}

	/*for testing data*/
	cout<<"extract testing data..."<<endl;

	for(int i = 0;i<template_size;i++)
	{
		cout<<"the "<<i<<"th template"<<endl;

		vector<vector<int> > temp_template;

		string one_template = template_set_->get(i);
		char * ptr = const_cast<char *>(one_template.c_str());

		int test_sent_size = data_->test_size();
		for(int j = 0;j<test_sent_size;j++)
		{
			if((j+1) % 100 == 0)
			{
				cout<<".";
				if((j+1) %5000 == 0)
					cout<<endl;
			}

			vector<int > temp_sent;

			vector<vector<string> > sent = data_->get_test(j);
			int word_len = sent.size();
			for(int pos=0;pos<word_len;pos++)
			{
				string one_feature;
				apply_template(one_feature,ptr,pos,sent);

				int idx = -1; // this feature does not appear in training data
				if(featuremap_[i].find(one_feature))
				{
					idx = featuremap_[i].find_idx(one_feature);
				}				
				temp_sent.push_back(idx);
			}
			temp_template.push_back(temp_sent);
		}
		test_features_.push_back(temp_template);
	}
}

//////////////////////////////////////////////////////////////////////////
void HMSVM_Extractor::combine()
{
	int start = 0;
	int combine_size = featuremap_.size();
	for(int i = 0;i<combine_size;i++)
	{
		cout<<"combining template "<<i<<"..."<<endl;
		start_pos_.push_back(start);
		BiDirectionMap fm = featuremap_[i];
		combinedmap_.push_back(fm);
		start += fm.size();
		group_size.push_back(fm.size());

		vector<vector<int> > itrain_features = train_features_[i];
		int train_sent_len = itrain_features.size();

		vector<vector<int> >itest_features = test_features_[i];
		int test_sent_len = itest_features.size();

		if(combined_train_features_.size() == 0) // initialize
		{
			for(int j = 0;j<train_sent_len;j++)
			{
				vector<vector<int> > temp1;
				for(size_t k=0;k<itrain_features[j].size();k++)
				{
					vector<int> temp2;
					temp1.push_back(temp2);
				}
				combined_train_features_.push_back(temp1);
			}

			for(int j = 0;j<test_sent_len;j++)
			{
				vector<vector<int> > temp1;
				for(size_t k=0;k<itest_features[j].size();k++)
				{
					vector<int> temp2;
					temp1.push_back(temp2);
				}
				combined_test_features_.push_back(temp1);
			}
		}

		for(int j = 0;j<train_sent_len;j++)
		{
			vector<int> words = itrain_features[j];
			for(size_t k=0;k<words.size();k++)
			{
				combined_train_features_[j][k].push_back(itrain_features[j][k] + start_pos_[i]);
			}
		}
		
		for(int j = 0;j<test_sent_len;j++)
		{
			vector<int> words = itest_features[j];
			for(size_t k=0;k<words.size();k++)
			{
				int idx = itest_features[j][k];
				if(idx != -1)
					combined_test_features_[j][k].push_back(idx + start_pos_[i]);
			}
		}
	}	
}


void HMSVM_Extractor::save_train_file(const char * train_file)
{
	ofstream ofs_train(train_file);

	// head information for groups
	int head_size = start_pos_.size();
	ofs_train<<"# "<<head_size<<endl;
	for(int i = 0;i<head_size;i++)
	{
		ofs_train<<"#"<<start_pos_[i]+1<< " "<<group_size[i]<<endl;
		cout<<"#"<<start_pos_[i]+1<< " "<<group_size[i]<<endl;
	}

	int sent_size = combined_train_features_.size();
	cout<<"train_file size="<< sent_size<<endl;
	for(int i = 0;i<sent_size;i++)
	{

		if((i+1) % 100 == 0)
		{
			cout<<".";
			if((i+1) %5000 == 0)
				cout<<endl;
		}

		vector<vector<string> > original = data_->get_train(i);

		vector<vector<int> > sent = combined_train_features_[i];
		vector<int> label_seq = train_labels_[i];
		int word_size = sent.size();

		for(int j = 0;j<word_size;j++)
		{
			int label = label_seq[j];
			vector<int> features = sent[j];

			ofs_train<<label+1<<" "; // label start from 1
			ofs_train<<"qid:"<<i+1<<" "; //sentence starts from 1
			for(size_t k = 0;k<features.size();k++)
			{
				ofs_train<<features[k] + 1<<":1 "; // feature idx start from 1
			}
			ofs_train<<"# ";
			ofs_train<<original[j][0];
			ofs_train<<endl;
		}
	}

	ofs_train.close();
}

void HMSVM_Extractor::save_test_file(const char * test_file)
{
	ofstream ofs_train(test_file);

	int sent_size = combined_test_features_.size();
	cout<<"save test file: size="<< sent_size<<endl;

	for(int i = 0;i<sent_size;i++)
	{
		vector<vector<string> > original = data_->get_test(i);

		vector<vector<int> > sent = combined_test_features_[i];
		vector<int> label_seq = test_labels_[i];
		int word_size = sent.size();

		for(int j = 0;j<word_size;j++)
		{
			int label = label_seq[j];
			vector<int> features = sent[j];

			ofs_train<<label+1<<" "; //label starts from 1
			ofs_train<<"qid:"<<i+1 << " "; //sentence starts from 1
			for(size_t k = 0;k<features.size();k++)
			{
				ofs_train<<features[k] + 1<<":1 "; // feature idx start from 1
			}
			ofs_train<<"# ";
			ofs_train<<original[j][0];
			ofs_train<<endl;
		}
	}

	ofs_train.close();
}


void HMSVM_Extractor::save_map(const char * map_file)
{
	ofstream ofs(map_file);
	cout<<"save map..."<<endl;

	/*label map and frequency*/
	ofs<<"# label map"<<endl;
	int size = labelmap_.size();
	ofs<<"size="<<size<<endl;
	for(int i = 0;i<size;i++)
		ofs<<labelmap_.get(i).key_ <<" "<<labelmap_.get(i).freq_<<endl;

	/*features and frequency*/
	ofs << "# features"<<endl;
	size = combinedmap_.size();
	ofs<<"size="<<size<<endl;
	for(int i = 0;i<size;i++)
		ofs<<combinedmap_.get(i).key_<<" "<<combinedmap_.get(i).freq_<<endl;

	ofs.close();
}

