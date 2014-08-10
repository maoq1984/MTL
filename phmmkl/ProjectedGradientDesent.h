#ifndef __PROJECTED_GRANDIENT_DESCENT__
#define __PROJECTED_GRANDIENT_DESCENT__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
using namespace std;

class Dynamic_Square_Matrix{
public:
	Dynamic_Square_Matrix(int max_size){
		this->max_size = max_size;
		size = 0;
		sm = new double*[max_size];
		for(int i = 0;i<max_size;++i){
			sm[i] = new double[max_size];
			for(int j=0;j<max_size;j++){
				sm[i][j] = 0.0;
			}				
		}
	}

	~Dynamic_Square_Matrix(){
		for(int i =0;i<max_size;++i){
			delete []sm[i];
		}
		delete []sm;
	}

	void increaseMatrix(int i, int j, double val){
		int max_idx = max(i,j);
		if(max_idx+1 > size)
			size = max_idx+1;
		sm[i][j] = val;
		sm[j][i] = val;
	}

	void pruneMatrix(int idx){
		for(int row=0;row<idx;row++){
			for(int i=idx;i<size-1;++i){
				sm[row][i] = sm[row][i+1]; // upper triangle
				sm[i][row] = sm[i+1][row]; // lower triangle
			}
		}

		for(int i=idx+1;i<size;++i){ 
			for(int j=idx+1;j<size;++j){
				sm[i-1][j-1] = sm[i][j];
			}
		}
		size = size-1;
	}

	// alpha' Q alpha
	double alphaQalpha(double * alpha){
		double val = 0.0;
		for(int i = 0;i<size;++i){
			for(int j = 0;j<size;++j){
				val += sm[i][j] * alpha[i] * alpha[j];
			}
		}
		return val;
	}

	//ret + weight .* Q * alpha = ret
	void Qalpha(double *alpha, double weight, double *&ret){
		double * tmp_ret = new double[size];
		for(int i =0;i<size;++i){
			tmp_ret[i] = 0.0;
			for(int j=0;j<size;++j){
				tmp_ret[i] += sm[i][j] * alpha[j];
			}
			tmp_ret[i] *= weight;
			ret[i] += tmp_ret[i];
		}
		delete[]tmp_ret;
	}

	void print_Matrix(){
		for(int i=0;i<size;++i){
			for(int j=0;j<size;++j){
				//cout<<sm[i][j]<<"\t";
				printf("%0.4f\t",sm[i][j]);
			}
			cout<<endl;
		}
	}

private:
	int max_size;
	int size;
	double **sm;
};


class PGD_Solver
{
public:
	PGD_Solver(int ngroup, double C, double p,int max_size);
	void increaseRest(double b){
		++numvar;
		c.push_back(-b);
	}

	void increaseQ(int qidx,int i,int j, double val){Q[qidx]->increaseMatrix(i,j,val);}	
	double get_mu(int gidx)	{return mu[gidx]; }
	double get_alpha(int vidx){ return alpha[vidx]; }
	double get_c(int i) {return -c[i]; }
	double get_zero_count(int i){return zero_count[i];}
	
	void prune_cut(int *prune_idx,int size,int *remain_idx, int remain_size);
	void print_results();
	void plot_all_matrix();
	void plot_matrix(int gi);
	void optimize();
	void project_onto_simplex(double * &v, int n);
	double get_objective(double *alpha);
	~PGD_Solver();

private:
	double *alpha;
	double *mu;

	int ngroup;
	int numvar;

	double C; // cost

	vector<double> c; // linear term
	Dynamic_Square_Matrix **Q;

	double p;

	vector<int> zero_count;
};


#endif