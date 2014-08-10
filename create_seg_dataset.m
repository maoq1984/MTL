function create_seg_dataset

template_file = 'seg.template';
flag = 0; % 0: word segmentation, 1: named entity recogintion

train_file_prefix = 'pku_training';
test_gold_test_prefix = 'pku_test_gold';

train_file = sprintf('seg/%s.utf8',train_file_prefix);
test_file = sprintf('seg/%s.utf8',test_gold_test_prefix);
crf_train_file = sprintf('seg/%s.crf',train_file_prefix);
crf_test_file = sprintf('seg/%s.crf',test_gold_test_prefix);
svm_train_file = sprintf('seg/%s.svm',train_file_prefix);
svm_test_file = sprintf('seg/%s.svm',test_gold_test_prefix);
svm_map_file = sprintf('seg/%s.map',train_file_prefix);

command = sprintf('StandardFileFormat.exe %s %s %s %s %s %s %s %s %d',train_file,test_file,template_file,crf_train_file,crf_test_file,svm_train_file,svm_test_file,svm_map_file,flag);

dos(command);



