#include "hmmkl.h"
#include <ctime>

int main(int argc, char * argv[])
{
	//const char * hmsvm_train_file = "D:/Projects/sequence_labeling/TNN-rebuttal/hmmkl_reducecut/data/esp_train.svm";
	//const char * hmsvm_test_file = "D:/Projects/sequence_labeling/TNN-rebuttal/hmmkl_reducecut/data/esp_testa.svm";
	//const char * hmsvm_test_output = "D:/Projects/sequence_labeling/TNN-rebuttal/hmmkl_reducecut/data/esp_testa_svm.outtags";
	//const char * hmsvm_weight_output = "D:/Projects/sequence_labeling/TNN-rebuttal/hmmkl_reducecut/data/eps_testa_svm.weight";

	//double p = 1.1;
	//double C = 10;
	//double eps = 0.01;
	//int maxiter = 500;

	if(argc != 9)
	{
		cerr<<"command: hmmkl hmmkl_train_file hmmkl_test_file hmmkl_test_output hmmkl_weight_output C eps maxiter p"<<endl;
		cerr<<"p should be larger than 1"<<endl;
		return 0;
	}

	const char * hmsvm_train_file = argv[1];
	const char * hmsvm_test_file = argv[2];
	const char * hmsvm_test_output = argv[3];
	const char * hmsvm_weight_output = argv[4];
	double C = atof(argv[5]);
	double eps = atof(argv[6]);
	int maxiter = atoi(argv[7]);
	double p = atof(argv[8]);


	Document train_doc;
	GroupSet group_set;
	read_train_doc(hmsvm_train_file,group_set,train_doc);

	time_t start, end;
	start = clock();

	HMMKL hmmkl(&train_doc,&group_set,C,eps,maxiter); // C x the number of sentence
	hmmkl.train(p);
//	hmmkl.train();

	end = clock();

	hmmkl.print_mu();
	hmmkl.print_mu(hmsvm_weight_output);

	clear(train_doc);
	clear(group_set);

	Document test_doc;
	read_test_doc(hmsvm_test_file,test_doc);
	hmmkl.predict(&test_doc,hmsvm_test_output);

	clear(test_doc);

	cout<<"\ntraining time:"<<(end - start)/1000.0<<"seconds"<<endl;
}