#include "hmmkl.h"
#include <math.h>

HMMKL::HMMKL(Document * doc, GroupSet *group_set,double C, double eps, int maxiter)
:eps_(eps),maxiter_(maxiter)
{
	doc_ = doc;
	gset_ = group_set;
	__nclas = doc_->nclas_;

	C_ = C * doc_->sent_size_;

	if(doc_->nfeat_ < gset_->nfeat_)
	{
		cout<<"training data has different number of features info:"<<doc_->nfeat_<<" header:"<<gset_->nfeat_<<endl;
		doc_->nfeat_ = gset_->nfeat_;
	}		

	fidx2group = new int[doc_->nfeat_+1]; // fidx and group all start from 1
	for(int i = 0;i<gset_->num_groups_;i++)
	{
		for(int j = 0;j<gset_->group_set_[i].ele_size_;j++)
		{
			int fidx = gset_->group_set_[i].group_[j];
			fidx2group[fidx] = i+1;
		}
	}
	/*
	fidx2group = new FGNode*[doc_->nfeat_+1]; // the 0th do not use
	for(int i = 0;i<gset_->num_groups_;i++)
	{
		for(int j=0;j<gset_->group_set_[i].ele_size_;j++)
		{
			int fidx = gset_->group_set_[i].group_[j];
			FGNode *p = new FGNode;
			p->gidx_ = fidx;
			p->next = fidx2group[fidx];
			fidx2group[fidx] = p;
		}
	}
	*/


	emit = new WeightVector(doc_->nfeat_,doc_->nclas_); //initialize w=0
	trans = new WeightVector(doc_->nclas_,doc_->nclas_);
	nump = 0;
	peptr = new double *[maxiter_];
	ptptr = new double *[maxiter_];

	//qcqp_solver = new MyQCQP(gset_->num_groups_ + 1, C_); // plus one transmit group, MyQCQP should automatically generate \sum alpha <= C constraint

	q = 0.0;
	loss = 0.0;
	mvy = new int*[doc_->sent_size_];
	for(int i = 0;i<doc_->sent_size_;i++)
	{
		mvy[i] = new int[doc_->doc_[i].token_size_];
	}

	mu = new double[gset_->num_groups_+1]; // keep the value of the weight for each template
}

HMMKL::~HMMKL()
{	
	delete emit;
	delete trans;
}

void HMMKL::train(double p){
	int slen = doc_->sent_size_;
	int glen = gset_->num_groups_;
	int nclas = doc_->nclas_;

	vector<double> error;
	PGD_Solver *pSover = new PGD_Solver(glen+1,C_,p,maxiter_);

	find_most_violated_y(); // compute mvy, q, loss
	double min_fval = numeric_limits<double>::max();//C_ * loss /slen;
	// initializing
	int iter = 1;
	while(true)
	{
		// calculate p by using most violated mvy at the position of nump 
		peptr[nump] = new double[emit->nweight_];
		for(int i = 0;i<emit->nweight_;i++) peptr[nump][i] = 0.0;
		ptptr[nump] = new double[trans->nweight_];
		for(int i = 0;i<trans->nweight_;i++) ptptr[nump][i] = 0.0;

		for(int i = 0;i<slen;i++)
		{
			Sentence sent = doc_->doc_[i];
			for(int j = 0;j<sent.token_size_;j++)
			{
				Token token = sent.sent_[j];
				int y = token.y_;
				int ys = mvy[i][j];
				for(int k=0;k<token.fvec_size_;k++) // emission
				{
					int fidx = token.fvec_[k].fidx_;
					double val = token.fvec_[k].value_;

					int idx_y = emit->get_idx(fidx,y);
					int idx_ys = emit->get_idx(fidx,ys); 
					peptr[nump][idx_y] += val;
					peptr[nump][idx_ys] -= val;
				}

				if(j>0) // transition
				{
					int pre_y = sent.sent_[j-1].y_;
					int pre_ys = mvy[i][j-1];

					int idx_yy = trans->get_idx(pre_y,y);
					int idx_yys = trans->get_idx(pre_ys,ys);
					ptptr[nump][idx_yy] += 1;
					ptptr[nump][idx_yys] -= 1;
				}
			}
		}

		for(int i=0;i<emit->nweight_;i++) peptr[nump][i] *= (-1.0/slen);
		for(int i=0;i<trans->nweight_;i++) ptptr[nump][i] *= (-1.0/slen); 
		++nump;

		// update Q matrix, q
		double *cur_pe = peptr[nump-1];
		double *cur_pt = ptptr[nump-1];
		for(int i = 0;i<nump;i++)
		{
			// emission
			double *pre_pe = peptr[i];
			for(int g=0;g<gset_->num_groups_;g++)
			{
				double Qg_cur_i = 0.0;
				for(int j = 0;j<gset_->group_set_[g].ele_size_;j++) //inner product on each group
				{
					int fidx = gset_->group_set_[g].group_[j];
					for(int k=1;k<=doc_->nclas_;k++)
					{
						int idx = trans->get_idx(fidx,k);
						Qg_cur_i += cur_pe[idx] * pre_pe[idx];
					}
				}

				if(i == nump - 1)
					Qg_cur_i += 1e-10;

				pSover->increaseQ(g,nump-1,i,Qg_cur_i);
			}

			// transition
			double *pre_pt = ptptr[i];
			double Q_cur_i = 0.0;
			for(int j=0;j<trans->nweight_;j++)
				Q_cur_i+=(pre_pt[j] * cur_pt[j]);

			if(i == nump-1)
				Q_cur_i += 1e-10;

			pSover->increaseQ(gset_->num_groups_,nump-1,i,Q_cur_i);
		}
		pSover->increaseRest(q/slen);

		//pSover->plot_all_matrix();

		pSover->optimize();

		//pSover->print_results();

		/////////////////////////////////////////////////////////////////////////////////
		// prune the unnecessary cut
		double sum_alpha = 0.0;
		for(int i = 0;i<nump;i++) sum_alpha+=pSover->get_alpha(i);

		int *prune = new int[nump];
		int prune_size = 0;
		int *remain = new int[nump];
		int remain_size = 0;
		for(int i =0;i<nump;i++)
		{
			//cout<<pSover->get_alpha(i)<<" ";

			//if(pSover->get_alpha(i)/sum_alpha  < 1e-10)
			if(pSover->get_alpha(i)  < 1e-10 && pSover->get_zero_count(i) >= 5) // modify for loss terminate criterion
			{
				prune[prune_size++] = i;
				delete []peptr[i];
				delete []ptptr[i];
			}
			else
				remain[remain_size++] = i;
		}
		//cout<<endl;
		
		if(prune_size > 0)
		{
			//compressing peptr and ptptr
			for(int i = 0;i<remain_size;i++)
			{
				peptr[i] = peptr[remain[i]];
				ptptr[i] = ptptr[remain[i]];
			}

			nump = remain_size;

			//pSover->plot_matrix(0);
			pSover->prune_cut(prune,prune_size,remain,remain_size);
			//pSover->plot_matrix(0);
		}

		delete []prune;
		delete []remain;
		/////////////////////////////////////////////////////////////////////////////////
		// update the new weights of emission and transition
		for(int k=0;k<emit->nweight_;k++)
		{
			double w = 0.0;
			for(int r=0;r<nump;r++)
			{
				double alpha_r = pSover->get_alpha(r);
				w+= (alpha_r * peptr[r][k]);
			}
			int fidx, y;
			emit->get_fidx_y(k,fidx,y);
			int gidx = fidx2group[fidx];
			w *= pSover->get_mu(gidx-1); // this implementation assume that one feature cannot belong to different group. so each feature has one mu.
			emit->weights_[k] = -w;
		}
	
		double mu_trans = pSover->get_mu(gset_->num_groups_);
		for(int k = 0;k<trans->nweight_;k++)
		{
			double w = 0.0;
			for(int r=0;r<nump;r++)
			{
				double alpha_r = pSover->get_alpha(r);
				w+= (alpha_r * ptptr[r][k]);
			}
			w *= mu_trans;
			trans->weights_[k] = -w;
		}

		// find most violated mvy
		find_most_violated_y();

		// the upper bound
		double upper_bound = loss / slen;
		
		// the lower bound
		double lower_bound = 0.0;
		for(int t=0;t<nump;t++)
		{
			double tmp_obj = 0.0;
			for(int i=0;i<emit->nweight_;i++) 
				tmp_obj+= emit->weights_[i] * peptr[t][i];
			for(int i=0;i<trans->nweight_;i++) 
				tmp_obj+= trans->weights_[i] * ptptr[t][i];
			tmp_obj = tmp_obj + pSover->get_c(t);

			if(lower_bound < tmp_obj)
				lower_bound = tmp_obj;
		}

		// stop criterion
		double gap = upper_bound - lower_bound;
		error.push_back(gap);

		cout<<"iter:"<<iter<<" gap="<<gap
			<<" upper_bound="<<upper_bound<<" lower_bound="<<lower_bound 
			<<" numcut= "<<nump<<endl;

		if(iter >=3){ // require the gap < eps at least 3 times
			int temp_size = error.size();
			if(error[temp_size-2]<eps_ && error[temp_size-3]< eps_ && gap < eps_ || iter >= maxiter_){
				if(iter <= maxiter_)
					cout<<"total "<< iter <<" iterations to reach eps-optimal"<<endl;
				else
					cout<<"reach to maximal iterations"<<endl;
				break;
			}
		}

		//if(iter >= maxiter_)
		//{
		//	cout<<"reach to maximal iterations"<<endl;
		//	break;
		//}


		iter = iter + 1;
	}

	// release memory
	for(int i = 0;i<doc_->sent_size_;i++)
		delete []mvy[i];
	delete []mvy;

	for(int i = 0;i<nump;i++)
	{
		delete []peptr[i];
		delete []ptptr[i];
	}
	delete []peptr;
	delete []ptptr;

	delete []fidx2group;

	for(int i = 0;i<=gset_->num_groups_;i++)
		mu[i] = pSover->get_mu(i);

	delete pSover;
}

void HMMKL::train()
{
	int slen = doc_->sent_size_;
	int glen = gset_->num_groups_;
	int nclas = doc_->nclas_;

	MyQCQP *qcqp_solver = new MyQCQP(gset_->num_groups_ + 1, C_); // plus one transmit group, MyQCQP should automatically generate \sum alpha <= C constraint

	find_most_violated_y(); // compute mvy, q, loss
	double min_fval = numeric_limits<double>::max();//C_ * loss /slen;
	// initializing
	int iter = 1;
	while(true)
	{
		// calculate p by using most violated mvy at the position of nump 
		peptr[nump] = new double[emit->nweight_];
		for(int i = 0;i<emit->nweight_;i++) peptr[nump][i] = 0.0;
		ptptr[nump] = new double[trans->nweight_];
		for(int i = 0;i<trans->nweight_;i++) ptptr[nump][i] = 0.0;

		for(int i = 0;i<slen;i++)
		{
			Sentence sent = doc_->doc_[i];
			for(int j = 0;j<sent.token_size_;j++)
			{
				Token token = sent.sent_[j];
				int y = token.y_;
				int ys = mvy[i][j];
				for(int k=0;k<token.fvec_size_;k++) // emission
				{
					int fidx = token.fvec_[k].fidx_;
					double val = token.fvec_[k].value_;

					int idx_y = emit->get_idx(fidx,y);
					int idx_ys = emit->get_idx(fidx,ys); 
					peptr[nump][idx_y] += val;
					peptr[nump][idx_ys] -= val;
				}

				if(j>0) // transition
				{
					int pre_y = sent.sent_[j-1].y_;
					int pre_ys = mvy[i][j-1];

					int idx_yy = trans->get_idx(pre_y,y);
					int idx_yys = trans->get_idx(pre_ys,ys);
					ptptr[nump][idx_yy] += 1;
					ptptr[nump][idx_yys] -= 1;
				}
			}
		}

		for(int i=0;i<emit->nweight_;i++) peptr[nump][i] *= (-1.0/slen);
		for(int i=0;i<trans->nweight_;i++) ptptr[nump][i] *= (-1.0/slen); 
		++nump;

		// update Q matrix, q
		double *cur_pe = peptr[nump-1];
		double *cur_pt = ptptr[nump-1];
		for(int i = 0;i<nump;i++)
		{
			// emission
			double *pre_pe = peptr[i];
			for(int g=0;g<gset_->num_groups_;g++)
			{
				double Qg_cur_i = 0.0;
				for(int j = 0;j<gset_->group_set_[g].ele_size_;j++) //inner product on each group
				{
					int fidx = gset_->group_set_[g].group_[j];
					for(int k=1;k<=doc_->nclas_;k++)
					{
						int idx = trans->get_idx(fidx,k);
						Qg_cur_i += cur_pe[idx] * pre_pe[idx];
					}
				}

				if(i == nump - 1)
					Qg_cur_i += 1e-10;

				qcqp_solver->increaseQ(g,nump-1,i,Qg_cur_i);
			}

			// transition
			double *pre_pt = ptptr[i];
			double Q_cur_i = 0.0;
			for(int j=0;j<trans->nweight_;j++)
				Q_cur_i+=(pre_pt[j] * cur_pt[j]);

			if(i == nump-1)
				Q_cur_i += 1e-10;

			qcqp_solver->increaseQ(gset_->num_groups_,nump-1,i,Q_cur_i);
		}
		qcqp_solver->increaseRest(q/slen);

	//	qcqp_solver->plot_all_matrix();

		//solve QCQP
		qcqp_solver->optimize();

	//	qcqp_solver->print_results();

		/////////////////////////////////////////////////////////////////////////////////
		// prune the unnecessary cut
		double sum_alpha = 0.0;
		for(int i = 0;i<nump;i++) sum_alpha+=qcqp_solver->get_alpha(i);

		int *prune = new int[nump];
		int prune_size = 0;
		int *remain = new int[nump];
		int remain_size = 0;
		for(int i =0;i<nump;i++)
		{
			//cout<<qcqp_solver->get_alpha(i)<<" ";

			if(qcqp_solver->get_alpha(i)/sum_alpha  < 1e-6)
			//if(qcqp_solver->get_alpha(i)  < 1e-10) // modify for loss termiate criterion
			{
				prune[prune_size++] = i;
				delete []peptr[i];
				delete []ptptr[i];
			}
			else
				remain[remain_size++] = i;
		}
		//cout<<endl;
		
		if(prune_size > 0)
		{
			//compressing peptr and ptptr
			for(int i = 0;i<remain_size;i++)
			{
				peptr[i] = peptr[remain[i]];
				ptptr[i] = ptptr[remain[i]];
			}

			nump = remain_size;

			qcqp_solver->prune_cut(prune,prune_size,remain,remain_size);
		}

		delete []prune;
		delete []remain;
		/////////////////////////////////////////////////////////////////////////////////
		// update the new weights of emission and transition
		for(int k=0;k<emit->nweight_;k++)
		{
			double w = 0.0;
			for(int r=0;r<nump;r++)
			{
				double alpha_r = qcqp_solver->get_alpha(r);
				w+= (alpha_r * peptr[r][k]);
			}
			int fidx, y;
			emit->get_fidx_y(k,fidx,y);
			int gidx = fidx2group[fidx];
			w *= qcqp_solver->get_mu(gidx-1); // this implementation assume that one feature cannot belong to different group. so each feature has one mu.
			emit->weights_[k] = -w;
		}
	
		double mu_trans = qcqp_solver->get_mu(gset_->num_groups_);
		for(int k = 0;k<trans->nweight_;k++)
		{
			double w = 0.0;
			for(int r=0;r<nump;r++)
			{
				double alpha_r = qcqp_solver->get_alpha(r);
				w+= (alpha_r * ptptr[r][k]);
			}
			w *= mu_trans;
			trans->weights_[k] = -w;
		}

		// find most violated mvy
		find_most_violated_y();

		// the upper bound
		double upper_bound = loss / slen;
		
		// the lower bound
		double lower_bound = 0.0;
		for(int t=0;t<nump;t++)
		{
			double tmp_obj = 0.0;
			for(int i=0;i<emit->nweight_;i++) 
				tmp_obj+= emit->weights_[i] * peptr[t][i];
			for(int i=0;i<trans->nweight_;i++) 
				tmp_obj+= trans->weights_[i] * ptptr[t][i];
			tmp_obj = tmp_obj + qcqp_solver->get_c(t);

			if(lower_bound < tmp_obj)
				lower_bound = tmp_obj;
		}

		// stop criterion
		double gap = upper_bound - lower_bound;

		cout<<"iter:"<<iter<<" gap="<<gap
			<<" upper_bound="<<upper_bound<<" lower_bound="<<lower_bound 
			<<" numcut= "<<nump<<endl;

		if(gap < eps_ || iter >= maxiter_)
		{
			if(iter >= maxiter_)
				cout<<"total "<< iter <<" iterations to reach eps-optimal"<<endl;
			else
				cout<<"reach to maximal iterations"<<endl;
			break;
		}

		//// compute convergence conditions
		//double reg = 0.0;
		//for(int g = 0;g<glen;g++)
		//{
		//	double tmp_reg = 0.0;
		//	for(int i=0;i<gset_->group_set_[g].ele_size_;i++)
		//	{
		//		int fidx = gset_->group_set_[g].group_[i];
		//		for(int k=1;k<nclas;k++)
		//		{
		//			double w = emit->get_weight(fidx,k);
		//			tmp_reg += (w * w);
		//		}
		//	}
		//	reg += sqrt(tmp_reg);
		//}

		//double temp_w = 0.0;
		//for(int i=0;i<trans->nweight_;i++) temp_w += (trans->weights_[i] *trans->weights_[i]);
		//reg += sqrt(temp_w);

		//reg = 0.5 * reg * reg;

		//// the upper bound
		//double upper_bound = reg + C_ * loss / slen;
		//if(min_fval > upper_bound)
		//	min_fval = upper_bound;

		//// the lower bound
		//double lower_bound = 0.0;
		//for(int t=0;t<nump;t++)
		//{
		//	double tmp_obj = 0.0;
		//	for(int i=0;i<emit->nweight_;i++) 
		//		tmp_obj+= emit->weights_[i] * peptr[t][i];
		//	for(int i=0;i<trans->nweight_;i++) 
		//		tmp_obj+= trans->weights_[i] * ptptr[t][i];
		//	tmp_obj = reg + C_ * (tmp_obj + qcqp_solver->get_c(t));

		//	if(lower_bound < tmp_obj)
		//		lower_bound = tmp_obj;
		//}

		//// stop criterion
		//double gap = min_fval - lower_bound;

		//cout<<"iter:"<<iter<<" gap="<<gap
		//	<<" upper_bound="<<min_fval<<" lower_bound="<<lower_bound <<" ratio="<< (min_fval-lower_bound)/min_fval
		//	<<" numcut= "<<nump<<endl;

		//if(gap / min_fval < eps_ || iter >= maxiter_)
		//{
		//	if(iter >= maxiter_)
		//		cout<<"total "<< iter <<" iterations to reach eps-optimal"<<endl;
		//	else
		//		cout<<"reach to maximal iterations"<<endl;
		//	break;
		//}

		iter = iter + 1;
	}

	// release memory
	for(int i = 0;i<doc_->sent_size_;i++)
		delete []mvy[i];
	delete []mvy;

	for(int i = 0;i<nump;i++)
	{
		delete []peptr[i];
		delete []ptptr[i];
	}
	delete []peptr;
	delete []ptptr;

	/*
	for(int i = 0;i<doc_->nfeat_;i++)
	{
		FGNode * p = fidx2group[i];
		while(p != NULL)
		{
			FGNode * temp = p;
			p = p->next;
			delete temp;
		}
	}
	*/
	delete []fidx2group;

	for(int i = 0;i<=gset_->num_groups_;i++)
		mu[i] = qcqp_solver->get_mu(i);

	delete qcqp_solver;
}

// compute mvy, q, loss
void HMMKL::find_most_violated_y()
{
	loss = 0.0;
	q = 0.0;

	int nclas = doc_->nclas_;
	for(int i = 0;i<doc_->sent_size_;i++)
	{
		Sentence sent = doc_->doc_[i];
		int token_size = sent.token_size_;
		// build lattice
		ViterbiNode ** lattice = new ViterbiNode *[token_size];

		//Viterbi algorithm
		for(int t=0;t<token_size;t++)
		{
			Token tokens = sent.sent_[t];
			lattice[t] = new ViterbiNode[nclas];
			for(int k=0;k<nclas;k++)
			{
				lattice[t][k].y = k+1;
				lattice[t][k].delta = 0.0;
				lattice[t][k].pre = NULL;
				
				int target_y = tokens.y_;
				double temp_b = 0.0;
				if(target_y != lattice[t][k].y) // Delta(y,y')
					temp_b = 1.0;
				for(int j = 0;j<tokens.fvec_size_;j++)
				{
					int fidx = tokens.fvec_[j].fidx_;
					double val = tokens.fvec_[j].value_;
					double w = emit->get_weight(fidx,lattice[t][k].y);
					temp_b += (val * w);
				}
				lattice[t][k].b = temp_b;
			}

			// forward pass
			if(t == 0)
			{
				for(int k = 0;k<nclas;k++) 
					lattice[t][k].delta = lattice[t][k].b;
			}
			else 
			{
				for(int m=0;m<nclas;m++)
				{
					int cur_y = lattice[t][m].y;

					double temp_max = -numeric_limits<double>::max();
					int temp_max_idx = 0;
					
					for(int n=0;n<nclas;n++)
					{
						int pre_y = lattice[t-1][n].y;
						double temp_delta = trans->get_weight(pre_y,cur_y) + lattice[t-1][n].delta;
						if(temp_delta > temp_max)
						{
							temp_max = temp_delta;
							temp_max_idx = n;
						}
					}

					lattice[t][m].delta = temp_max + lattice[t][m].b;
					lattice[t][m].pre = &lattice[t-1][temp_max_idx];
				}
			} // Viterbi
		}// sentence

		// back pass and get mvy for this sentence
		int max_sent_idx = 0;
		double max_sent_loss = lattice[token_size-1][max_sent_idx].delta;		
		for(int k = 1;k<nclas;k++)
		{
			if(max_sent_loss < lattice[token_size-1][k].delta)
			{
				max_sent_loss = lattice[token_size-1][k].delta;
				max_sent_idx = k;
			}
		}

		// update mvy
		double sent_q = 0.0;
		ViterbiNode *p = &lattice[token_size-1][max_sent_idx];
		int pos = token_size - 1;
		while(p != NULL)
		{
			mvy[i][pos] = p->y;
			
			int target_y = sent.sent_[pos].y_;
			if(target_y != p->y) 
				sent_q+=1.0;
			
			p = p->pre;
			pos--;
		}

		double const_sent_loss = 0.0;
		for(int t=0;t<token_size;t++)
		{
			Token tokens = sent.sent_[t];
			int y = tokens.y_;
			const_sent_loss += lattice[t][y-1].b;
			if(t >0)
			{
				int pre_y = sent.sent_[t-1].y_;
				const_sent_loss += trans->get_weight(pre_y,y);
			}
		}

		loss += (max_sent_loss - const_sent_loss);
		q += sent_q;

		//delete memory
		for(int t = 0;t<token_size;t++)
			delete []lattice[t];
		delete []lattice;

	}//doc
	
}

void HMMKL::predict(Document *test_doc,const char* save_file)
{
	ofstream ofs(save_file);
	
	int nclas = __nclas;
	for(int i = 0;i<test_doc->sent_size_;i++)
	{
		cout<<".";
		Sentence sent = test_doc->doc_[i];
		int token_size = sent.token_size_;
		// build lattice
		ViterbiNode ** lattice = new ViterbiNode *[token_size];

		//Viterbi algorithm
		for(int t=0;t<token_size;t++)
		{
			Token tokens = sent.sent_[t];
			lattice[t] = new ViterbiNode[nclas];
			for(int k=0;k<nclas;k++)
			{
				lattice[t][k].y = k+1;
				lattice[t][k].delta = 0.0;
				lattice[t][k].pre = NULL;

				double temp_b = 0.0;
				for(int j = 0;j<tokens.fvec_size_;j++)
				{
					int fidx = tokens.fvec_[j].fidx_;
					double val = tokens.fvec_[j].value_;
					double w = emit->get_weight(fidx,lattice[t][k].y);
					temp_b += (val * w);
				}
				lattice[t][k].b = temp_b;
			}

			// forward pass
			if(t == 0)
			{
				for(int k = 0;k<nclas;k++) 
					lattice[t][k].delta = lattice[t][k].b;
			}
			else 
			{
				for(int m=0;m<nclas;m++)
				{
					int cur_y = lattice[t][m].y;

					double temp_max = -numeric_limits<double>::max();
					int temp_max_idx = 0;

					for(int n=0;n<nclas;n++)
					{
						int pre_y = lattice[t-1][n].y;
						double temp_delta = trans->get_weight(pre_y,cur_y) + lattice[t-1][n].delta;
						if(temp_delta > temp_max)
						{
							temp_max = temp_delta;
							temp_max_idx = n;
						}
					}

					lattice[t][m].delta = temp_max + lattice[t][m].b;
					lattice[t][m].pre = &lattice[t-1][temp_max_idx];
				}
			} // Viterbi
		}// sentence

		// back pass and get mvy for this sentence
		int max_sent_idx = 0;
		double max_sent_loss = lattice[token_size-1][max_sent_idx].delta;		
		for(int k = 1;k<nclas;k++)
		{
			if(max_sent_loss < lattice[token_size-1][k].delta)
			{
				max_sent_loss = lattice[token_size-1][k].delta;
				max_sent_idx = k;
			}
		}

		// update mvy
		int * vec_y = new int[token_size];

		ViterbiNode *p = &lattice[token_size-1][max_sent_idx];
		int pos = token_size-1;
		while(p != NULL)
		{
			//ofs<< p->y <<" "<< sent.sent_[pos].y_ <<endl;
			vec_y[pos] = p->y;
			p = p->pre;
			pos--;
		}

		//output
		for(int i = 0;i<token_size;i++)
		{
			ofs<<vec_y[i]<<endl;
		}


		//delete memory
		delete []vec_y;

		for(int t = 0;t<token_size;t++)
			delete []lattice[t];
		delete []lattice;
	}

	ofs.close();
}