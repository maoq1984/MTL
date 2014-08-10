#ifndef __MYQCQP_H__
#define __MYQCQP_H__

#include <stdlib.h>
#include "mosek.h"

#include <iostream>
#include <vector>
using namespace std;

class MyQCQP
{
public:
	MyQCQP(int ngroup,double C);
	
	void increaseRest(double b);

	void increaseQ(int qidx, int i, int j, double val);

	void optimize();

	void print_results();

	void plot_matrix(int gi)
	{
		cout<<"-----the "<<gi<<"the matrix-----"<<endl;
		vector<MSKidxt> qsubii = qsubi[gi];
		vector<MSKidxt> qsubji = qsubj[gi];
		vector<double> qvali = qval[gi];

		for(int i = 0;i<qvali.size();i++)
		{
			cout<<qvali[i]<<"\t";
			if(i<qvali.size()-1 && qsubii[i+1] != qsubii[i])
				cout<<endl;
		}
		cout<<endl;
	}

	void plot_all_matrix()
	{
		int gsize = numcon-1;
		for(int i = 0;i<gsize;i++)
			plot_matrix(i);
	}

	double get_mu(int gidx)
	{
		return mu[gidx];
	}
	
	double get_alpha(int vidx)
	{
		return alpha[vidx+1];
	}

	double get_c(int i)
	{
		return -c[i+1];
	}

	void prune_cut(int *prune_idx,int size,int *remain_idx, int remain_size);

	~MyQCQP()
	{
		delete []mu;
		if(alpha != NULL)
			delete []alpha;
	}
private:
	int numcon;
	int numvar;

	double *alpha;
	double *mu;

	vector<double> c;
	
	vector<MSKboundkeye> bkc;
	vector<double> blc;
	vector<double> buc;

	vector<MSKboundkeye> bkx;
	vector<double> blx;
	vector<double> bux;

	vector<MSKlidxt> aptrb;
	vector<MSKlidxt> aptre;
	vector<MSKidxt> asub;
	vector<double> aval;

	vector<vector<MSKidxt> > qsubi;
	vector<vector<MSKidxt> > qsubj;
	vector<vector<double> > qval;	
};


#endif