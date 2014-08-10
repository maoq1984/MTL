#include "myQCQP.h"

MyQCQP::MyQCQP(int ngroup,double C)
{
	numcon = ngroup + 1; //ngroup Quadratic constraint, 1 linear constraint
	numvar = 1;

	//init for one variable theta, no most violated constraint
	c.push_back(1.0);

	for(int i = 0;i<numcon;i++)
	{
		bkc.push_back(MSK_BK_UP);
		blc.push_back(-MSK_INFINITY);
		if(i<numcon-1)
			buc.push_back(0.0);
		else
			buc.push_back(C);
	}

	bkx.push_back(MSK_BK_LO);
	blx.push_back(0.0);
	bux.push_back(+MSK_INFINITY);

	aptrb.push_back(0);
	aptre.push_back(numcon-1);
	for(int i=0;i<numcon-1;i++)
	{
		asub.push_back(i);
		aval.push_back(-1.0);
	}

	for(int i = 0;i<numcon-1;i++)
	{
		vector<MSKidxt> tmp_qsubi;
		qsubi.push_back(tmp_qsubi);
		vector<MSKidxt> tmp_qsubj;
		qsubj.push_back(tmp_qsubj);
		vector<double> tmp_qval;
		qval.push_back(tmp_qval);
	}

	alpha = NULL;
	mu = new double[numcon];
}

void MyQCQP::increaseRest(double b)
{
	numvar++;
	c.push_back(-b);

	bkx.push_back(MSK_BK_LO);
	blx.push_back(0.0);
	bux.push_back(+MSK_INFINITY);

	aval.push_back(1.0);
	asub.push_back(numcon-1);
	int len = asub.size();
	aptre.push_back(len);
	aptrb.push_back(len-1);
}

void MyQCQP::increaseQ(int qidx, int i, int j, double val)
{
	qsubi[qidx].push_back(i+1);
	qsubj[qidx].push_back(j+1);
	qval[qidx].push_back(val);
}


void MyQCQP::optimize()
{
	// resize alpha
	if(alpha != NULL)
		delete []alpha;
	alpha = new double[numvar];

	double *c_ = new double[numvar];
	for(int i = 0;i<numvar;i++) c_[i] = c[i];

	MSKboundkeye *bkc_ = new MSKboundkeye[numcon];
	double * blc_ = new double[numcon];
	double * buc_ = new double[numcon];
	for(int i = 0;i<numcon;i++)
	{
		bkc_[i] = bkc[i];
		blc_[i] = blc[i];
		buc_[i] = buc[i];
	}

	MSKboundkeye *bkx_ = new MSKboundkeye[numvar];
	double * blx_ = new double[numvar];
	double * bux_ = new double[numvar];
	for(int i = 0;i<numvar;i++)
	{
		bkx_[i] = bkx[i];
		blx_[i] = blx[i];
		bux_[i] = bux[i];
	}

	MSKlidxt *aptrb_ = new MSKlidxt[aptrb.size()];
	for(size_t i = 0;i<aptrb.size();i++) aptrb_[i] = aptrb[i];
	MSKlidxt * aptre_ = new MSKlidxt[aptre.size()];
	for(size_t i = 0;i<aptre.size();i++) aptre_[i] = aptre[i];
	MSKidxt * asub_ = new MSKidxt[asub.size()];
	for(size_t i = 0;i<asub.size();i++) asub_[i] = asub[i];
	double *aval_ = new double[aval.size()];
	for(size_t i = 0;i<aval.size();i++) aval_[i] = aval[i];

	MSKrescodee r;
	MSKenv_t env;
	MSKtask_t task;
	r = MSK_makeenv(&env,NULL,NULL,NULL,NULL);
	r = MSK_initenv(env);

	if(r == MSK_RES_OK)
	{
		r = MSK_maketask(env,numcon,numvar,&task);
		if(r == MSK_RES_OK)
			r = MSK_append(task,MSK_ACC_CON,numcon);

		if(r == MSK_RES_OK)
			r = MSK_append(task,MSK_ACC_VAR, numvar);

		for(int j = 0;j<numvar && r== MSK_RES_OK;j++)
		{
			if(r == MSK_RES_OK)
				r = MSK_putcj(task,j,c_[j]);

			if(r == MSK_RES_OK)
				r = MSK_putbound(task,MSK_ACC_VAR,j,bkx_[j],blx_[j],bux_[j]);

			if(r == MSK_RES_OK)
				r = MSK_putavec(task,MSK_ACC_VAR,j,aptre_[j] - aptrb_[j], asub_ + aptrb_[j],aval_+aptrb_[j]);
		}

		for(int i=0;i<numcon  && r== MSK_RES_OK;i++)
		{
			r = MSK_putbound(task,MSK_ACC_CON,i,bkc_[i],blc_[i],buc_[i]);
		}


		delete []c_;
		delete []bkx_;
		delete []blx_;
		delete []bux_;
		delete []aptrb_;
		delete []aptre_;
		delete []asub_;
		delete []aval_;
		delete []bkc_;
		delete []blc_;
		delete []buc_;


		for(int i=0;i<numcon-1 && r== MSK_RES_OK;i++) // numcon-1 quadratic constraints
		{
			int nzero = qsubi[i].size();
			MSKidxt * qsubi_ = new MSKidxt[nzero];
			MSKidxt * qsubj_ = new MSKidxt[nzero];
			double * qval_ = new double[nzero];
			for(int m = 0;m<nzero;m++)
			{
				qsubi_[m] = qsubi[i][m];
				qsubj_[m] = qsubj[i][m];
				qval_[m] = qval[i][m];
			}

			if(r == MSK_RES_OK)
				r = MSK_putqconk(task,i,nzero,qsubi_,qsubj_,qval_);

			delete []qsubi_;
			delete []qsubj_;
			delete []qval_;
		}


		if(r == MSK_RES_OK)
			r = MSK_putobjsense(task,MSK_OBJECTIVE_SENSE_MINIMIZE);

		if(r == MSK_RES_OK)
		{
			MSKrescodee trmcode;
			r = MSK_optimizetrm(task,&trmcode);

			MSK_getsolutionslice(task,MSK_SOL_ITR, MSK_SOL_ITEM_XX,0,numvar,alpha);

			MSK_getsolutionslice(task,MSK_SOL_ITR,MSK_SOL_ITEM_SUC,0,numcon,mu);
		}
		MSK_deletetask(&task);
	}
	MSK_deleteenv(&env);
}

void MyQCQP::prune_cut(int *prune_idx,int size,int *remain_idx, int remain_size)
{
	int prune_numvar = numvar;
	// prune_idx should sort from small to large value
	for(int i = size-1;i>=0;i--)
	{
		int pidx = prune_idx[i] + 1;

		//erase the element of c
		vector <double>::iterator c_iter = c.begin() + pidx;
		c.erase(c_iter);

		//erase the element of variable bound, since all the elements are the same, just pop_back one element
		bkx.pop_back();
		blx.pop_back();
		bux.pop_back();

		//erase A
		vector<MSKlidxt>::iterator aptrb_iter = aptrb.begin() + pidx;
		vector<MSKlidxt>::iterator aptre_iter = aptre.begin() + pidx;
		int pos = *aptrb_iter;
		aptrb.erase(aptrb_iter);
		aptre.erase(aptre_iter);
		for(int j=pidx;j<aptrb.size();j++)
		{
			aptrb[j] --;
			aptre[j] --;
		}

		vector<MSKidxt>::iterator asub_iter= asub.begin() + pos;
		vector<double>::iterator aval_iter = aval.begin() + pos;
		asub.erase(asub_iter);
		aval.erase(aval_iter);

		//erase Q
		for(int g=0;g<numcon-1;g++)
		{
			vector<MSKidxt>::iterator qsubig_iter = qsubi[g].begin();
			vector<MSKidxt>::iterator qsubjg_iter = qsubj[g].begin();
			vector<double>::iterator qvalg_iter = qval[g].begin();
						
			for(int i =qsubi[g].size()-1;i>=0;i--)
			{
				if(qsubi[g][i] == pidx || qsubj[g][i] == pidx)
				{
					qsubi[g].erase(qsubig_iter+i);
					qsubj[g].erase(qsubjg_iter+i);
					qval[g].erase(qvalg_iter+i);
				}
				else if(qsubi[g][i] > pidx || qsubj[g][i]>pidx)
				{
					if(qsubi[g][i] > pidx)
						qsubi[g][i] --;
					
					if(qsubj[g][i]>pidx)
						qsubj[g][i] --;
				}
			}

		}

		// reduce the number of variables
		--prune_numvar;
	}

	//change alpha
	double *temp_alpha = new double[prune_numvar];
	temp_alpha[0] = alpha[0];
	for(int i = 0;i<remain_size;i++)
		temp_alpha[i+1] = alpha[remain_idx[i]+1];
	delete []alpha;
	alpha = temp_alpha;

	numvar = prune_numvar;
}


void MyQCQP::print_results()
{
	//cout<<"-------------------"<<numvar<<"----------------"<<endl;
	//cout<<"alpha:"<<endl;
	//for(int i = 1;i<numvar;i++)
	//	cout<<alpha[i]<<" ";
	//cout<<endl;

	cout<<"mu:"<<endl;
	for(int i = 0;i<numcon-1;i++)
		cout<<mu[i]<<" ";
	cout<<endl;
}
