#include "TemplateSet.h"

void TemplateSet::load_template(const char * template_file)
{
	ifstream ifs(template_file);
	string line;
	while (getline(ifs, line)) {
		if (!line.size() || (line.size() && (line[0] == ';' || line[0] == '#'))) 
			continue;

		size_t pos = line.find('=');

		size_t s1;
		for (s1 = pos+1; s1 < line.size() && isspace(line[s1]); s1++);
		std::string value = line.substr(s1, line.size() - s1);

		if(value[0] != 'B') // in our model, we implicitly use bi-gram, so here no need to explicitly obtain bigram template
		{
			template_set_.push_back(value);	
			cout<<value<<endl;
		}
	}
	ifs.close();
}


void TemplateSet::print_templates(ostream &os)
{
	int size = this->size();
	for(int i = 0;i<size;i++)
	{
		string one_template = this->get(i);
		os<<one_template<<endl;
	}
}