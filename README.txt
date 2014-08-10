This package implements the following paper
Qi Mao, Ivor W. Tsang. Efficient Multi-Template Learning for Structured Prediction. IEEE Transactions on Neural Networks and Learning Systems, 24(2): 248 - 261, Feb 2013.
If any problem, please contact with qi mao (maoq1984@gmail.com)

The folder TNNLS2013 includes the following files:

1) Chinese word segmentation data from http://www.sighan.org/bakeoff2005/
We preprocess them into the input format of CRF++ and SVMs, including as, cityu, msr, and pku, under the folder ./seg
Note: * can be either of four datasets.

*_training.utf8: the original training files
*_test_gold.utf8: the ogirinal testing files
*_training_words.utf8: the dictionary of Chinese words in the training data

After runing matlab script "create_seg_dataset.m", we can obtain the following files:
----------for CRF---------------
*_training.crf : the training data
*_test_gold.crf: the test data

----------for SVMs--------------
*_training.svm : the training data
*_test_gold.svm: the test data
*_training.map: the feature from string to integet map

Note: to run "create_seg_dataset.m", you also need to provide different template. Each template will generate different number of features for the same dataset for SVMs.

In this zip files, we provide the original data and preprocessed crf data. To obtain svm files with different templates, you need to run "create_seg_dataset.m". Since the preprocessed svm data is very large, we do not include them in this zip package.

2) To run the experiments, we need to prepare the following executive file:

a) CRF++-0.54, downloaded from http://crfpp.googlecode.com/svn/trunk/doc/index.html, and compile to obtain executive file crf_learn and crf_test
b) Compile win2linux.cpp and seg_converter.cpp to executive file win2linux and seg_convert. 
c) Two template files: seg.template and seg_large.template
d) compile hmmkl in different operating system. (Currently, we only test it in Windows). In order to be sucessfully, Mosek is required to solve QCQP problem from websit http://www.mosek.com/. The current directionary and stationary library for mosek is required to set correctly for Visual studio C++ (we use VS2008).

Note: we attached the executive files which are tested in 64 bit Windows system. To run on 32 bit Windows sytem, you need to recompile all the C++ codes.

3) evaluation needs 

a) run score

------------------------------------Example for L1-CRF in Linux System----------------------------
./win2linux seg/pku_training.crf seg/pku_training_linux.crf
./win2linux seg/pku_test_gold.crf seg/pku_test_gold_linux.crf
./crf_learn -a CRF-L1 -c 0.01 seg.template seg/pku_training_linux.crf seg/pku/pku_training_1_crf.model
./crf_test -m seg/pku/pku_training_1_crf.model seg/pku_test_gold_linux.crf > seg/pku/pku_test_gold_1_crf.outtags
./seg_converter seg/pku/pku_test_gold_1_crf.target seg/pku/pku_test_gold_1_crf.predict seg/pku/pku_test_gold_1_crf.outtags
./score seg/pku_training_words.utf8 seg/pku/pku_test_gold_1_crf.target seg/pku/pku_test_gold_1_crf.predict > seg/pku/pku_test_gold_1_crf.eval

The results on template "seg.template" and pku data with c=0.01 are shown in pku_test_gold_1_crf.eval. For large templates, we only need to change the template option to "seg_large.template".

The command for MTL^hmm is in the form of
                     "hmmkl hmmkl_train_file hmmkl_test_file hmmkl_test_output hmmkl_weight_output C eps maxiter".
The details for each parameter needs to check the C++ code.
----------------------------------Example for MTL^{hmm} in Window System------------------------------------------
hmmkl.exe seg/pku_training.svm seg/pku_test_gold.svm seg/pku/pku_test_gold_5_hmmkl.outtags seg/pku/pku_test_gold_5_hmmkl.weight 100 0.1 500
seg_converter seg/pku/pku_test_gold_5_hmmkl.target seg/pku/pku_test_gold_5_hmmkl.predict seg/pku_test_gold.crf seg/pku/pku_test_gold_5_hmmkl.outtags

After obtain the outtags file, the same evaluation process as in L1-CRF is used for the evaluation score. To obtain the results from different template, you need to run the above script with different svm files generated from "seg.template" or "seg_large.template" by running "create_seg_dataset.m".

4) phmmkl
This is the implementation of p-norm mixed regularization term for sequence labelling problem. It is similar to the parameter setting in hmmkl.
