#include "cic.hpp"
#include <iostream>

using namespace cic;

void ParametersGroup::add(const IAnyTypeParameter& parameter)
{
	m_parameters.push_back(std::unique_ptr<IAnyTypeParameter>(parameter.copy()));
}

void ParametersGroup::readPO(const boost::program_options::variables_map& clOpts)
{

}

void func()
{
    auto f = []() {
        std::cout << "Hello, cmake fan" << std::endl;
    };
    
    f();
}

double sqr(double x)
{
    return x*x;
}
