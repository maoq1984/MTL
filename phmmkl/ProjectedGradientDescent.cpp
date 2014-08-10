#include "ProjectedGradientDesent.h"
#include <float.h>


int cmp( const int &a, const int &b ){
	if( a > b )
		return 1;
	else
		return 0;
}

PGD_Solver::PGD_Solver(int ngroup, double C, double p,int max_size){
	this->ngroup = ngroup;
	this->C = C;
	this->p = p;

	numvar = 0;
	alpha = NULL;
	mu = new double[ngroup];

	Q = new Dynamic_Square_Matrix*[ngroup];
	for (int i = 0;i<ngroup;++i){
		Q[i] = new Dynamic_Square_Matrix(max_size);
	}

}

PGD_Solver::~PGD_Solver(){
	delete []mu;
	if(alpha != NULL)
		delete []alpha;
	for(int i = 0;i<ngroup;++i)
		delete Q[i];
	delete []Q;
}

void PGD_Solver::print_results()
{
	cout<<"mu:"<<endl;
	for(int i = 0;i<ngroup;i++)
		cout<<mu[i]<<" ";
	cout<<endl;
}

void PGD_Solver::plot_matrix(int gi){
	cout<<"-----the "<<gi<<"the matrix-----"<<endl;
	Q[gi]->print_Matrix();
}

void PGD_Solver::plot_all_matrix(){
	for(int i = 0;i<ngroup;i++)
		plot_matrix(i);
}

void PGD_Solver::prune_cut(int *prune_idx,int size,int *remain_idx, int remain_size){
	//change Q and c
	int prune_numvar = numvar;
	for(int i=size-1;i>=0;i--){
		int pidx = prune_idx[i];

		vector <double>::iterator c_iter = c.begin() + pidx;
		c.erase(c_iter);

		vector<int>::iterator zero_count_iter = zero_count.begin() + pidx;
		zero_count.erase(zero_count_iter);

		for(int g=0;g<ngroup;g++){
			Q[g]->pruneMatrix(pidx);		
		}		
		--prune_numvar;
	}

	//change alpha		
	double *temp_alpha = new double[prune_numvar];
	for(int i = 0;i<remain_size;i++)
		temp_alpha[i] = alpha[remain_idx[i]];
	delete []alpha;
	alpha = temp_alpha;

	numvar = prune_numvar;
}

void PGD_Solver::optimize(){
	double pstar = p/(p-1);
	double stepsize = 1e-6;

	zero_count.push_back(0); // new added cut is zero 

	//if(alpha != NULL) delete[]alpha;
	//alpha = new double[numvar];
	//for(int i = 0;i<numvar;i++){ // initialized by average value
	//	alpha[i] = 0.0;
	//}

	
	if(alpha == NULL){
		alpha = new double[numvar];
		for(int i = 0;i<numvar;i++){ // initialized by average value
			alpha[i] = 0.0;
		}
	}else{
		double * tmp_alpha = alpha;
		alpha = new double[numvar];
		for(int i = 0;i<numvar-1;i++)
			alpha[i] = tmp_alpha[i];
		alpha[numvar-1] = 0.0;
		delete []tmp_alpha;
	}

	// start projected gradient descent method
	double *grad = new double[numvar];
	double *aQa = new double[ngroup];
	double *old_mu = new double[ngroup];
	double old_obj = DBL_MAX;

	int iter = 1;
	while(1){
		// compute gradient		
		double tmp_sum = 0.0;
		double const_c = 0.0;
		for(int g=0;g<ngroup;g++){
			aQa[g] = Q[g]->alphaQalpha(alpha); // avoid numeric issue when aQa[g]=0

			const_c+= pow( aQa[g], 0.5*p/(p-1) );
			mu[g] = pow(aQa[g],0.5*(2-p)/(p-1));
			tmp_sum+= pow(aQa[g],pstar/2);
		}		
			
		const_c = pow(const_c, (p-2) /p);

		for(int g=0;g<ngroup;g++) mu[g] *= const_c;		

		// specially process pstar
		double max_alpha = alpha[0];
		for(int i = 1;i<numvar;i++){
			if(max_alpha < alpha[i]){
				max_alpha = alpha[i];
			}
		}

		for(int i = 0;i<numvar;++i) grad[i] = c[i];
		if(max_alpha > 1e-10){ // alpha != 0
			double weight_1 = pow( tmp_sum, 2/pstar-1);		
			for(int g=0;g<ngroup;g++){
				double weight_2 = pow(aQa[g], pstar/2 -1);
				Q[g]->Qalpha(alpha,weight_1*weight_2,grad);
			}	
		}		

		// find the proper stepsize by backtracking 
		double tmp_obj_0 = get_objective(alpha);
		double norm_grad_sq = 0.0;
		for(int i=0;i<numvar;i++) norm_grad_sq+=grad[i]*grad[i];
		double * tmp_alpha = new double[numvar];
		double tmp_t = 1.0; 
		int tmp_iter = 1;
		while(1){
			// calculate objective f(mid_alpha) where mid_alpha = alpha - tmp_t grad
			for(int i=0;i<numvar;++i) tmp_alpha[i] = alpha[i] - tmp_t * grad[i];
			double tmp_obj = get_objective(tmp_alpha);

			if(tmp_obj < tmp_obj_0 - 0.1 * tmp_t * norm_grad_sq || tmp_iter >= 40){
				delete []alpha;
				alpha = tmp_alpha; // gradient descent
				break;
			}
			tmp_t *= 0.5;
			++tmp_iter;
		}	
		
		// projection onto simplex
		project_onto_simplex(alpha,numvar);

		double obj = get_objective(alpha);		
		if(iter > 1){
			double diff_obj = abs((obj - old_obj) / old_obj);

			//if(iter % 10 == 0)
			//	cout <<"\titer="<<iter <<", obj="<<obj <<",robj="<<diff_obj <<endl;
			if(diff_obj < 1e-6){
				cout <<"\titer="<<iter <<", obj="<<obj <<",robj="<<diff_obj <<endl;
				break;
			}			
		}
		//terminate conditions iter > 1 && norm_mu < 1e-6 ||
		if( iter > 10000){
			break;
		}

		old_obj = obj;

		for(int i = 0;i<ngroup;i++)
			old_mu[i] = mu[i];
		++iter;
	}

	delete []grad;
	delete []aQa;
	delete []old_mu;

	// update count according to alpha
	for(int i = 0;i<numvar;i++){
		if(alpha[i] < 1e-10 ){ // candidate for remove
			zero_count[i] = zero_count[i] + 1;
		}else
			zero_count[i]=0;
	}
}

double PGD_Solver::get_objective(double *alpha){
	double pstar = p/(p-1);
	double obj = 0.0;
	for(int g=0;g<ngroup;++g){
		double tmp = Q[g]->alphaQalpha(alpha);
		obj += pow(tmp,pstar/2);
	}

	obj = pow(obj,2/pstar) * 0.5;

	for(int i = 0;i<numvar;i++){
		obj += alpha[i] * c[i];
	}
	return obj;
}

void PGD_Solver::project_onto_simplex(double * &v, int n){
	double *u = new double[n];
	for(int i = 0;i<n;i++){ 
		v[i] = v[i] >0 ? v[i]:0;
		u[i] = v[i];
	}

	sort(u,u+n,cmp); // descend
	double tmp = 0;
	double *sv = new double[n];
	bool *flag = new bool[n];
	for(int i =0;i<n;i++){
		tmp += u[i];
		sv[i] = tmp;
		flag[i] = (u[i] > (sv[i] - C)/(i+1));
	}

	int rho = n-1;
	for(int i=n-1;i>=0;i--){
		if(flag[i]){
			rho = i;
			break;
		}
	}

	for(int i = 0;i<n;i++){
		double theta = ((sv[rho]-C)/(rho+1)) >0 ? ((sv[rho]-C)/(rho+1)):0;
		v[i] = (v[i] - theta) > 0? (v[i] - theta) : 0;
	}

	delete []u;
	delete []sv;
	delete []flag;

}