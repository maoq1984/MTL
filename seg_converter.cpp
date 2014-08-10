#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
using namespace std;

// B=1. E=2, I=3

vector<vector<string> > transform(const char *crf_results)
{
	vector<vector<string> > results;
	ifstream ifs_crf(crf_results);
	char line[8192];

	vector<string> temp;
	while(!ifs_crf.eof())
	{
		ifs_crf.getline(line,sizeof(line));
		if(line[0] == '\0')
		{
			if(temp.size() > 0)
				results.push_back(temp);
			temp.clear();
			continue;
		}
		string tuple(line);
		temp.push_back(tuple);
	}

	return results;
}


vector<vector<string> > transform(const char*gold_crf, const char *label_file)
{
	vector<vector<string> > results;

	ifstream ifs_crf(gold_crf);
	ifstream ifs_label(label_file);


	char line[8192];
	int label;
	vector<string> temp;
	while(!ifs_crf.eof() && !ifs_label.eof())
	{
		ifs_crf.getline(line,sizeof(line));

		if(line[0] == '\0') //new line
		{
			if(temp.size() > 0)
				results.push_back(temp);
			temp.clear();
			continue;
		}

		ifs_label >> label;

		string tuple(line);
		if(label == 1)
		{
			tuple.append(" B");
		}else if(label == 2)
		{
			tuple.append(" I");
		}else if(label == 3)
		{
			tuple.append(" E");
		}else
		{
			cerr<<"error label"<<label<<endl;
		}
		temp.push_back(tuple);
	}

	ifs_crf.close();
	ifs_label.close();	
	return results;
}

void output_segment(const char* out_target, const char* out_predict, vector<vector<string> > & results)
{
	ofstream ofs_target(out_target);
	ofstream ofs_predict(out_predict);

	int num_sent = results.size();
	for(int i = 0;i<num_sent;i++)
	{
		vector<string> sent = results[i];
		int num_words = sent.size();
		
		string sent_target;
		string sent_predict;
		for(int j = 0;j<num_words;j++)
		{
			string line = sent[j];
			char * temp_line = const_cast<char *> (line.c_str());

			char *token =NULL;
			token = strtok(temp_line,"\t ");
			string chars(token);

			token = strtok(NULL,"\t ");
			if(token[0] == 'B')
			{
				sent_target.append(" ");
			}
			sent_target.append(chars);

			token = strtok(NULL,"\t ");
			if(token[0] == 'B')
			{
				sent_predict.append(" ");
			}
			sent_predict.append(chars);
			
		}
		ofs_target << sent_target <<endl;
		ofs_predict << sent_predict <<endl;
	}

	ofs_target.close();
	ofs_predict.close();
}

int main(int argc, char* argv[])
{

	const char *target_file = argv[1];
	const char *predict_file = argv[2];

	if(argc == 4)
	{
		const char *crf_results_file = argv[3];
		vector<vector<string> > results = transform(crf_results_file);
		output_segment(target_file,predict_file,results);
	}
	else if(argc == 5)
	{
		const char*gold_crf  = argv[3];
		const char *label_file = argv[4];
		vector<vector<string> > results = transform(gold_crf,label_file);
		output_segment(target_file,predict_file,results);
	}
	else
	{
		cerr<<"the formate is : target_file predict_file [crf_results_file / gold_crf label_file]\n";
	}

	//const char*gold_crf = "D:/Projects/sequence_labeling/results/as_test_gold.crf";
	//const char *label_file = "D:/Projects/sequence_labeling/results/as_test_gold.outtags";

	//const char *target_file = "D:/Projects/sequence_labeling/results/as_test_gold.target";
	//const char *predict_file = "D:/Projects/sequence_labeling/results/as_test_gold.predict";

	//vector<vector<string> > results = transform(gold_crf,label_file);
	//output_segment(target_file,predict_file,results);

	return 0;
}
