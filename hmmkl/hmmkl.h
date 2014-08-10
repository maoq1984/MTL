#include "hmmkl_struct.h"
#include "myQCQP.h"

#include <vector>
#include <iostream>
#include <limits>

using namespace std;

class WeightVector
{
public:
	WeightVector(int nfidx, int nclas):nfidx_(nfidx),nclas_(nclas)
	{
		nweight_ = nfidx_ * nclas_;
		weights_ = new double[nweight_];
		for(int i = 0;i<nweight_;i++) weights_[i] = 0.0;
	}

	~WeightVector()
	{
		delete []weights_;
	}

	// fidx and y are all started from 1
	int get_idx(int fidx, int y)
	{
		return (fidx -1) * nclas_ + y - 1;
	}

	double get_weight(int fidx,int y)
	{
		int idx = get_idx(fidx,y);
		return weights_[idx];
	}

	void set_weight(int fidx, int y, int w)
	{
		int idx = get_idx(fidx,y);
		weights_[idx] = w;
	}

	// idx starts from 0, but fidx and y starts from 1
	void get_fidx_y(int idx, int &fidx, int &y)
	{
		fidx = idx / nclas_ + 1;
		y = idx % nclas_ + 1;
	}

//private:
	int nfidx_;
	int nclas_;
	int nweight_;
	double * weights_;
};

struct FGNode
{
	int gidx_;
	FGNode * next;
};

struct ViterbiNode
{
	int y;
	double b;
	double delta;
	ViterbiNode * pre;
};

class HMMKL
{
public:
	HMMKL(Document * doc, GroupSet *group_set,double C, double eps, int maxiter);

	void train();

	void find_most_violated_y();

	void print_mu()
	{
		cout<<"group weight:"<<endl;
		for(int i = 0;i<=gset_->num_groups_;i++)
			cout<<mu[i]<<" ";
		cout<<endl;
	}

	void print_mu(const char * hmsvm_weight_output)
	{
		ofstream ofs(hmsvm_weight_output);
		for(int i = 0;i<=gset_->num_groups_;i++)
			ofs<<mu[i]<<endl;
		ofs.close();
	}

	void predict(Document *test_doc,const char* save_file);

	~HMMKL();

private:
	Document *doc_;
	GroupSet *gset_;
	//FGNode** fidx2group;
	int *fidx2group;

	WeightVector *emit;
	WeightVector *trans;

	MyQCQP * qcqp_solver;

	double C_;
	double eps_;
	int maxiter_;

	// temp variables
	double **peptr;
	double **ptptr;
	int nump;

	double q;
	double loss;
	int **mvy;

	double *mu;
	int __nclas;
};