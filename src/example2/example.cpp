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
	PreconfiguredOperations::addGeneralOptions(p);
	bool needRun;
	try {
		needRun = PreconfiguredOperations::quickReadConfiguration(
			p,
			{
				"/etc/example.conf",
				"~/example.conf"
			},
			argc,
			argv
		);
	} catch (std::exception &ex) {
		cout << "Error while reading configuration: " << ex.what() << endl;
	}

	if (!needRun)
		return 0;

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
