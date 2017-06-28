#include "cic.hpp"
#include <iostream>
#include <boost/property_tree/ini_parser.hpp>

using namespace cic;

ParametersGroup::ParametersGroup(const char* groupName, const char* description) :
		m_optionsDescr(description),
		m_groupName(groupName),
		m_description(description)
{
}

ParametersGroup::ParametersGroup(ParametersGroup&& pg) :
		m_optionsDescr(std::move(pg.m_optionsDescr)),
		m_groupName(std::move(pg.m_groupName)),
		m_description(std::move(pg.m_description)),
		m_parameters(std::move(pg.m_parameters))
{
}

const std::string& ParametersGroup::name()
{
	return m_groupName;
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
	if (!m_description.empty())
	{
		/// @todo Add # to every line in case of multiline
		stream << "# " << m_description << std::endl;
	}

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

Parameters::Parameters(const char* title) :
		m_allOptions(title)
{
}

void Parameters::addGroup(ParametersGroup&& pg)
{
	m_pgOwners.push_back(std::unique_ptr<ParametersGroup>(new ParametersGroup(std::move(pg))));
	addGroup(*m_pgOwners.back());
}

void Parameters::addGroup(ParametersGroup& pg)
{
	m_groups[pg.name()] = &pg;
	m_allOptions.add(pg.getOptionsDesctiption());
}

void Parameters::parseCmdline(int argc, const char** argv)
{
	namespace po = boost::program_options;
	po::store(po::parse_command_line(argc, argv, m_allOptions), m_vm);
	po::notify(m_vm);
	for (auto it=m_groups.begin(); it!=m_groups.end(); it++)
	{
		it->second->readPOVarsMap(m_vm);
	}
}

void Parameters::parseIni(const char* filename)
{
	try {
		boost::property_tree::ini_parser::read_ini(filename, m_pt);
	}
	catch(boost::property_tree::ini_parser::ini_parser_error &exception)
	{
		throw std::runtime_error(std::string("Parsing error in ") + exception.filename()
				+ ":" + std::to_string(exception.line()) + " - " + exception.message());
	}
	catch(boost::property_tree::ptree_error &exception)
	{
		throw std::runtime_error(std::string("Parsing error in ") + exception.what());
	}
	catch(...)
	{
		throw std::runtime_error("Unknown parsing error");
	}
	for (auto it=m_groups.begin(); it!=m_groups.end(); it++)
	{
		it->second->readPT(m_pt);
	}
}

ParametersGroup& Parameters::operator[](const std::string& groupName)
{
	return *m_groups[groupName];
}
