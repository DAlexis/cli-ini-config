#include "cic.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <pwd.h>

using namespace cic;

template<>
void Parameter<bool>::addToPO(boost::program_options::options_description& od) const
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		od.add_options()
			(m_name.c_str(), m_description.c_str());
	}
}

template<>
bool Parameter<bool>::getFromPO(const boost::program_options::variables_map& clOpts)
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		if (clOpts.count(m_name.c_str()) != 0)
		{
			try {
				m_value = clOpts[m_name.c_str()].as<bool>();
			} catch (std::exception &) {
				m_value = true;
			}
			m_setByUser = true;
			m_isInitialized = true;
		}
	}
	return initialized();
}

template<>
void Parameter<bool>::initNoDefault()
{
	m_value = false;
	m_isInitialized = true;
}

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

void Parameters::parseCmdline(int argc, const char* const * argv)
{
	m_vm.clear();
	namespace po = boost::program_options;
	try
	{
		po::store(po::parse_command_line(argc, argv, m_allOptions), m_vm);
		po::notify(m_vm);
	}
	catch (po::error& e)
	{
		throw (std::runtime_error(std::string("Command line parsing error: ") + e.what()));
	}

	for (auto it=m_groups.begin(); it!=m_groups.end(); it++)
	{
		it->second->readPOVarsMap(m_vm);
	}
}

void Parameters::parseIni(const char* filename)
{
	std::string fname = SystemUtils::replaceTilta(filename);
	try {
		boost::property_tree::ini_parser::read_ini(fname.c_str(), m_pt);
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

void Parameters::parseIni(const std::vector<std::string>& variants, const std::string& suffix)
{
	std::string filename = SystemUtils::probeFiles(variants, suffix);
	if (filename == "")
		throw std::runtime_error("Cannot file configuration file");

	parseIni(filename.c_str());
}

void Parameters::cmdlineHelp(std::ostream& stream)
{
	stream << m_allOptions;
}

void Parameters::writeIni(std::ostream& stream)
{
	for (auto it=m_groups.begin(); it != m_groups.end(); it++)
	{
		it->second->writeIniItem(stream);
	}
}

void Parameters::writeIni(const char* filename)
{
	std::ofstream iniFile(filename, std::ios::out);
	writeIni(iniFile);
}

const boost::program_options::variables_map& Parameters::variablesMap()
{
	return m_vm;
}

const boost::property_tree::ptree& Parameters::propertyTree()
{
	return m_pt;
}

ParametersGroup& Parameters::operator[](const std::string& groupName)
{
	return *m_groups[groupName];
}

std::string SystemUtils::homeDir()
{
	const char *homedir;

	if ((homedir = getenv("HOME")) == NULL) {
	    homedir = getpwuid(getuid())->pw_dir;
	}
	return std::string(homedir);
}

std::string SystemUtils::replaceTilta(const std::string& source)
{
	std::string result = source;
	size_t pos = result.find(std::string("~"));
	if (pos != result.npos)
		result.replace(pos, 1, homeDir());

	return result;
}

std::string SystemUtils::probeFiles(const std::vector<std::string>& variants, const std::string& suffix)
{
	for (auto& it : variants)
	{
		std::string fullName = it + suffix;

		if (boost::filesystem::exists(fullName))
		{
			return fullName;
		}
	}
	return "";
}
