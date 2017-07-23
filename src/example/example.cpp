#include "cic.hpp"

#include <iostream>
#include <fstream>

using namespace std;
using namespace cic;

int main(int argc, char** argv)
{
	Parameters p(
		"All parameters for your program",
		ParametersGroup(
			"General", // Group name
			"General program options", // Group description
			Parameter<string>("load-ini", "Load settings from ini file", ParamterType::cmdLine),
			Parameter<string>("save-ini", "Save setting to ini file", ParamterType::cmdLine),
			Parameter<bool>("help", "Print help", ParamterType::cmdLine)
		),
		ParametersGroup(
			"Input",
			"Input parameters",
			Parameter<double>("k", "Value of k", 1.23),
			Parameter<double>("b", "Value of b", 9.87)
		),
		ParametersGroup(
			"Interface",
			"User interface parameters",
			Parameter<std::string>("greeter", "String parameter", "Hi, user.")
		)
	);
	p.parseCmdline(argc, argv);
	if (p["General"].get<bool>("help"))
	{
		p.cmdlineHelp(std::cout);
		return 0;
	}

	if (p["General"].initialized("load-ini"))
	{
		p.parseIni(p["General"].get<string>("load-ini").c_str());
	}
	// Reparsing to override ini file containment
	p.parseCmdline(argc, argv);

	// Saving current values to ini file
	if (p["General"].initialized("save-ini"))
	{
		p.writeIni(p["General"].get<string>("save-ini").c_str());
	}

	// Main program
	cout << p["Interface"].get<string>("greeter") << endl;
	cout << "kx+b=0" << endl;
	double k = p["Input"].get<double>("k");
	double b = p["Input"].get<double>("b");
	if (k == 0.0)
		cout << "x is any number" << endl;
	else
		cout << "x = " << -b/k << endl;
	return 0;
}
