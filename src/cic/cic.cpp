#include "cic.hpp"
#include <iostream>

using namespace cic;

ParametersGroup::ParametersGroup(const std::string& groupName) :
		m_groupName(groupName)
{
}

void ParametersGroup::add(const IAnyTypeParameter& parameter)
{
	m_parameters[parameter.name()] = std::unique_ptr<IAnyTypeParameter>(parameter.copy());
	m_parameters[parameter.name()]->addToPO(m_optionsDescr);
}

void ParametersGroup::readPOVarsMap(const boost::program_options::variables_map& clOpts)
{
	for (auto &it : m_parameters)
	{
		it.second->getFromPO(clOpts);
	}
}

const boost::program_options::options_description& ParametersGroup::getOptionsDesctiption()
{
	return m_optionsDescr;
}

bool ParametersGroup::readPT(const boost::property_tree::ptree& pt)
{
	if (pt.count(m_groupName) == 0)
		return areAllInitialized();

	auto& node = pt.get_child(m_groupName);

	bool allInitialized = true;
	for (auto &it : m_parameters)
	{
		allInitialized = allInitialized && it.second->getFromPT(node);
	}
	return allInitialized;
}

void ParametersGroup::writeIniItem(std::ostream& stream)
{
	stream << std::endl << "[" << m_groupName << "]" << std::endl;
	for (auto &it : m_parameters)
	{
		it.second->writeIniItem(stream);
	}
}

IAnyTypeParameter& ParametersGroup::getInterface(const std::string& name)
{
	auto it = m_parameters.find(name);
	if (it == m_parameters.end())
		throw std::runtime_error(std::string("Parameter ") + name + " is not contained in group " + m_groupName);

	return *m_parameters[name].get();
}

bool ParametersGroup::areAllInitialized()
{
	for (auto &it : m_parameters)
	{
		if (!it.second->initialized())
			return false;
	}
	return true;
}


