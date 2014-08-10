#ifndef __TEMPLATESET_H__
#define __TEMPLATESET_H__

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;


class TemplateSet
{
public:
	TemplateSet(){}
	~TemplateSet(){}

	void load_template(const char * template_file);

	int size(){return template_set_.size();}
	string get(int i){ return template_set_[i];}
	void print_templates(ostream &os);
private:
	vector<string> template_set_;
};

#endif