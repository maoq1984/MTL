#include "SFileFormat.h"

#include <math.h>
//////////////////////////////////////////////////////////////////////////

vector<int> RandPerm::run_rand()
{
	srand( (unsigned)time(NULL));
	for(int i = 0;i<size_;i++) perm_.push_back(i);

	vector<int> results;
	int rest = size_;
	for(int i = 0;i<rand_size_;i++)
	{
		int r = rand() % rest;

		// swap the value in r and rest-1
		results.push_back(perm_[r]);

		int temp = perm_[r];
		perm_[r] = perm_[rest-1];
		perm_[rest-1] = temp;

		--rest;
	}		

	return results;
}


//////////////////////////////////////////////////////////////////////////
void SFileFormat::load_file(const char *filename, vector<string> & sents)
{
	char line[8192];
	ifstream ifs(filename);
	while(!ifs.eof())
	{
		ifs.getline(line,sizeof(line));
		//if(line[0] != '\0') remove for ner, added for seg
		{
			string one_sent(line);
			sents.push_back(one_sent);
		}
	}
	ifs.close();
}

void SFileFormat::load(const char *train_file, const char *test_file)
{
	train_sents.clear();
	load_file(train_file,train_sents);

	test_sents.clear();
	load_file(test_file,test_sents);

	parse();
}

void SFileFormat::load(const char *file, double ratio)
{
	vector<string> sents;
	load_file(file,sents);

	int size = sents.size();
	int rand_size = (int)(size * ratio);

	RandPerm one(size,rand_size);
	one.run_rand();
	vector<int> perms = one.get();

	for(int i=0;i<size;i++)
	{
		if(i < size - rand_size) // train
			train_sents.push_back(sents[perms[i]]);
		else
			test_sents.push_back(sents[perms[i]]);

	}

	parse();
}

void SFileFormat::save_file(const char *file,vector<vector<vector<string> > > &doc)
{
	ofstream ofs(file);
	int sent_size = doc.size();
	for(int i = 0;i<sent_size;i++)
	{
		vector<vector<string> > sent =doc[i];
		int row_size = sent.size();
		for(int j = 0;j<row_size;j++)
		{
			vector<string> cols = sent[j];
			int col_size = cols.size();
			for(int k = 0;k<col_size;k++)
				ofs<<cols[k]<<" ";
			ofs<<endl;
		}
		ofs<<endl;
	}
}

//////////////////////////////////////////////////////////////////////////

void Seg_SFileFormat::parse(vector<string> sent, vector<vector<vector<string> > > &doc)
{
	int num_of_sent = sent.size();
	for(int i = 0;i<num_of_sent;i++)
	{

		if((i+1) % 100 == 0)
		{
			cout<<".";
			if((i+1) %5000 == 0)
				cout<<endl;
		}

		vector<vector<string> > sent_matrix;
		string sent_str = sent[i];

		char *token =NULL;
		char *next_token = NULL;
		char *line = const_cast<char *>(sent_str.c_str());
		token = strtok_s(line,"\t ",&next_token);
		while(token !=NULL)
		{
			// different Chinese character encoding
//			vector<string> words = gbstring(token);
			vector<string> words = uft8string(token);

			//BIE coding
			int size = words.size();
			if(size >= 1)
			{
				vector<string> temp;
				temp.push_back(words[0]);
				temp.push_back("B");
				sent_matrix.push_back(temp);

				for(int k=1;k<size-1;k++)
				{
					vector<string> temp;
					temp.push_back(words[k]);
					temp.push_back("I");
					sent_matrix.push_back(temp);
				}

				if(size-1 > 0)
				{
					vector<string> temp;
					temp.push_back(words[size-1]);
					temp.push_back("E");
					sent_matrix.push_back(temp);
				}
			}

			token = strtok_s(NULL,"\t ",&next_token);
		}
		doc.push_back(sent_matrix);
	}

	// set column information
	vector< vector<string> > temp_sent = train_doc[0];
	vector<string> word = temp_sent[0];
	column = word.size() - 1; // the last column is label
}


void NER_CONLL_SFileFormat::parse(vector<string> sent, vector<vector<vector<string> > > &doc)
{
	vector<vector<string> > one_sent;

	for(int i = 0;i<sent.size();i++)
	{
		if((i+1) % 1000 == 0)
		{
			cout<<".";
			if((i+1) %50000 == 0)
				cout<<endl;
		}

		vector<string> tokens;
		string line = sent[i];
		if(line.size() == 0) // new sentence
		{
			doc.push_back(one_sent);
			one_sent.clear();
			continue;
		}

		char * temp_line = const_cast<char *> (line.c_str());
		char *token = NULL;

		token = strtok(temp_line,"\t ");
		while(token != NULL)
		{
			string token_str(token);
			tokens.push_back(token_str);
			token = strtok(NULL,"\t ");
		}

		one_sent.push_back(tokens);
	}

	// set column information
	vector< vector<string> > temp_sent = doc[0];
	vector<string> word = temp_sent[0];
	column = word.size() - 1; // the last column is label

}

//////////////////////////////////////////////////////////////////////////
vector<string> gbstring(const char * str)
{
	vector<string> words;
	string temp(str);
	int size = temp.size();
	int i = 0;
	while(i < size)
	{
		string temp1;
		unsigned char b = str[i];
		if(b >= 0xA1 && b<=0xF7)
		{
			temp1 = temp.substr(i,2);
			i += 2;
		}
		else
		{
			temp1 = temp.substr(i,1);
			i++;
		}
		words.push_back(temp1);
	}
	return words;
}

vector<string> uft8string(const char* pBuffer)
{
	vector<string> words;
	string temp(pBuffer);
	int size = temp.size();

	int i = 0;
	while (i < size)
	{
		string temp1;
		unsigned char b = pBuffer[i];
		if (b < 0x80) // (10000000): 值小于0x80的为ASCII字符
		{
			temp1 = temp.substr(i,1);
			i++;
		}
		else if (b < (0xC0)) // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
		{
			break;
		}
		else if (b < (0xE0)) // (11100000): 此范围内为2字节UTF-8字符
		{
			if (i >= size - 1)
				break;
			if ((pBuffer[i+1] & (0xC0)) != 0x80)
			{
				break;
			}
			temp1 = temp.substr(i,2);
			i += 2;
		}
		else if (b < (0xF0)) // (11110000): 此范围内为3字节UTF-8字符
		{
			if (i >= size - 2)
				break;
			if ((pBuffer[i+1] & (0xC0)) != 0x80 || (pBuffer[i+2] & (0xC0)) != 0x80)
			{
				break;
			}
			temp1 = temp.substr(i,3);
			i += 3;
		}
		else
		{
			break;
		}

		words.push_back(temp1);
	}
	return words;
}
