#include <fstream>
#include <string>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{

	if(argc != 3)
	{
		cerr<<"command: input_file output_file"<<endl;
		return 0;
	}

	const char * input_file = argv[1];
	const char * output_file =argv[2];

	ifstream ifs(input_file);
	ofstream ofs(output_file);


	char line[8192];

	while(!ifs.eof())
	{
		ifs.getline(line,sizeof(line));

		string temp(line);
		temp = temp.substr(0,temp.size()-1);
		ofs<< temp<<endl;
	}

	ifs.close();
	ofs.close();
}
