#include "FeatureExtractor.h"

void main(int argc, char *argv[])
{
	if(argc != 10)
	{
		cerr<< "parameters: train_file test_file template_file crf_train_file crf_test_file svm_train_file svm_test_file map_file flag=[0 Word segment| 1 named entity]"<<endl;
		return;
	}

	char * train_file = argv[1];
	char *test_file = argv[2];
	char *template_file = argv[3];
	char *crf_train_file = argv[4];
	char *crf_test_file = argv[5];
	char *svm_train_file = argv[6];
	char *svm_test_file = argv[7];
	char *map_file= argv[8];
	int flag = atoi(argv[9]);

	//const char * train_file = "D:/Projects/sequence_labeling/code/preprocess/data/as_training.utf8";
	//const char * test_file = "D:/Projects/sequence_labeling/code/preprocess/data/as_testing_gold.utf8";

	//const char * crf_train_file = "D:/Projects/sequence_labeling/code/preprocess/data/as_training.crf";
	//const char * crf_test_file = "D:/Projects/sequence_labeling/code/preprocess/data/as_testing_gold.crf";

	//const char * svm_train_file = "D:/Projects/sequence_labeling/code/preprocess/data/msr_training.svm";
	//const char * svm_test_file = "D:/Projects/sequence_labeling/code/preprocess/data/msr_test_gold.svm";
	//const char * map_file = "D:/Projects/sequence_labeling/code/preprocess/data/msr_training.map";

	//const char * template_file = "D:/Projects/sequence_labeling/code/preprocess/data/seg.template";

	////const char * train_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/esp.train";
	////const char * test_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/esp.testa";
	////const char * crf_train_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/eps_train.crf";
	////const char * crf_test_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/eps_test.crf";
	////const char * svm_train_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/eps_train.svm";
	////const char * svm_test_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/eps_testa.svm";
	////const char * map_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/eps_train.map";
	////const char * template_file = "D:/Projects/sequence_labeling/dataset/ner-Conll2002/data/ner_template.txt";

	TemplateSet templateset;
	templateset.load_template(template_file);

	if(flag == 1)
	{
		NER_CONLL_SFileFormat seg;
		seg.load(train_file,test_file);
		seg.save(crf_train_file,crf_test_file);

		HMSVM_Extractor extractor(&templateset,&seg);
		extractor.combine();
		extractor.save(svm_train_file,svm_test_file,map_file);
	}
	else
	{
		Seg_SFileFormat seg;
		seg.load(train_file,test_file);
		seg.save(crf_train_file,crf_test_file);

		HMSVM_Extractor extractor(&templateset,&seg);
		extractor.combine();
		extractor.save(svm_train_file,svm_test_file,map_file);
	}

}