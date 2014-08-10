#include "hmmkl_struct.h"

void read_head(ifstream &ifs,GroupSet &gset)
{
	string line;
	getline(ifs, line);
	string size_str = line.substr(1,line.size());

	int group_size = atoi(size_str.c_str());
	gset.group_set_ = new Group[group_size];
	gset.num_groups_ = group_size;

	for(int i = 0;i<group_size;i++)
	{
		getline(ifs,line);
		string temp = line.substr(1,line.size());

		char *temp_str = const_cast<char *>(temp.c_str());
		char *token = NULL;
		token = strtok(temp_str,"\t ");
		int start_ = atoi(token);

		token = strtok(NULL,"\t ");
		int len_ = atoi(token);
		
		gset.group_set_[i].group_ = new int[len_];
		gset.group_set_[i].ele_size_ = len_;

		cout<<"#"<<start_<<" "<<len_<<endl;

		for(int j = 0;j<len_;j++) 
			gset.group_set_[i].group_[j] = start_ + j;
		if(i == group_size-1)
		{
			gset.nfeat_ = start_ + len_ - 1;
		}
	}
}

void read_features(ifstream &ifs, Document & doc)
{
	vector<vector<FidxValue> > x;
	vector<int> sent_start_pos;
	vector<int> sent_len;
	vector<int> labels;

	int sent_idx = 1;
	int line_number = 0;

	sent_start_pos.push_back(1);
	
	string line;
	while(!ifs.eof())
	{
		getline(ifs,line);
	
		if(line.size()==0) // the last line could include lost of empty line
			continue;

		line_number++;

		vector<FidxValue> temp_line;
		// remove all the comments
		int idx = line.find('#');
		if(idx != string::npos) 
			line = line.substr(0,idx);

		char * temp_str = const_cast<char *> (line.c_str());
		char *token = NULL;

		token = strtok(temp_str," \t:");
		int label = atoi(token);
		
		token = strtok(NULL," \t:");
		token = strtok(NULL," \t:");
		int cur_sent_idx = atoi(token);		

		if(sent_idx != cur_sent_idx)	// start a new sentence
		{
			sent_idx = cur_sent_idx;

			if(sent_idx % 1000 == 0)
			{
				cout<<sent_idx<<" ";
			}

			sent_start_pos.push_back(line_number);
		}

		while(1)
		{				
			token = strtok(NULL," \t:");	
			if(token == NULL) break;
			int fidx = atoi(token);			
			token = strtok(NULL," \t:");	
			double value = atof(token);
			FidxValue fv(fidx,value);
			temp_line.push_back(fv);
		}

		labels.push_back(label);
		x.push_back(temp_line);
	}

	line_number++;
	sent_start_pos.push_back(line_number);
	

	// store in doc
	doc.sent_size_ = sent_start_pos.size()-1;
	doc.doc_ = new Sentence[doc.sent_size_];
	doc.nclas_ = 0;
	doc.nfeat_ = 0;

	for(int i = 0;i<doc.sent_size_;i++)
	{
		int sent_start = sent_start_pos[i];
		int sent_end = sent_start_pos[i+1];
		doc.doc_[i].token_size_ = sent_end - sent_start;
		doc.doc_[i].sent_ = new Token[doc.doc_[i].token_size_];

		for(int j=0;j<doc.doc_[i].token_size_;j++)
		{
			int pos = sent_start + j; // start from 1
			vector<FidxValue> temp_x = x[pos-1];
			int temp_y = labels[pos-1];
			doc.doc_[i].sent_[j].y_ = temp_y;
			doc.doc_[i].sent_[j].fvec_size_ = temp_x.size();
			doc.doc_[i].sent_[j].fvec_ = new FidxValue[doc.doc_[i].sent_[j].fvec_size_];

			if(temp_y > doc.nclas_) doc.nclas_ = temp_y; // record the class encoding from 1 to maxclass

			for(int k=0;k<doc.doc_[i].sent_[j].fvec_size_;k++)
			{
				doc.doc_[i].sent_[j].fvec_[k].fidx_ = temp_x[k].fidx_;
				doc.doc_[i].sent_[j].fvec_[k].value_ = temp_x[k].value_;

				if(temp_x[k].fidx_ > doc.nfeat_) doc.nfeat_ = temp_x[k].fidx_;
			}
		}
	}
	cout<<"finished!"<<endl;
}

void clear(GroupSet &group_set)
{
	for(int i = 0;i<group_set.num_groups_;i++)
	{
		Group *g = &group_set.group_set_[i];
		delete []g->group_;
	}
	delete []group_set.group_set_;
}

void clear(Document &doc)
{
	int sent_size = doc.sent_size_;
	for(int i = 0;i<sent_size;i++)
	{
		int token_size = doc.doc_[i].token_size_;
		for(int j = 0;j<token_size;j++)
		{
			delete []doc.doc_[i].sent_[j].fvec_;
		}
		delete []doc.doc_[i].sent_;
	}
	delete []doc.doc_;
}

void read_train_doc(const char * train_file, GroupSet & gset, Document & train_doc)
{
	cout<<"\n read training document"<<endl;
	ifstream ifs(train_file);
	read_head(ifs,gset);
	read_features(ifs,train_doc);
	ifs.close();
}

void read_test_doc(const char * test_file, Document & test_doc)
{
	cout<<"\n read test document"<<endl;
	ifstream ifs(test_file);
	read_features(ifs,test_doc);
	ifs.close();
}


void print_struct(int sidx,Document & doc)
{
	Sentence sent = doc.doc_[sidx-1];
	for(int i = 0;i<sent.token_size_;i++)
	{
		Token token = sent.sent_[i];
		cout<<token.y_<<" qid:"<<sidx<<" ";

		for(int j = 0;j<token.fvec_size_;j++)
		{
			cout<<token.fvec_[j].fidx_<<":"<<token.fvec_[j].value_<<" ";
		}
		cout<<endl;
	}
}