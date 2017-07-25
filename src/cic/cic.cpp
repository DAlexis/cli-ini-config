#include "cic.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <pwd.h>

using namespace cic;

template<>
void Parameter<bool>::addToPO(boost::program_options::options_description& od, const std::string& prefix, bool defaultsNeeded) const
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		if (m_isInitialized && defaultsNeeded)
		{
			od.add_options()
				((prefix + m_name).c_str(), boost::program_options::value<bool>()->default_value(m_value), m_description.c_str());
		} else {
			od.add_options()
				((prefix + m_name).c_str(), m_description.c_str());
		}
	}
}

template<>
bool Parameter<bool>::getFromPO(const boost::program_options::variables_map& clOpts, const std::string& optionalPrefix)
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		auto tryName = [this, &clOpts](const std::string& name) -> bool {
			if (clOpts.count(name.c_str()) == 0)
				return false;

			try {
				m_value = clOpts[name.c_str()].as<bool>();
			} catch (std::exception &) {
				m_value = true;
			}
			m_setByUser = true;
			m_isInitialized = true;
			return true;
		};
		if (!tryName(optionalPrefix + m_name))
			tryName(m_name);
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
		m_groupName(groupName),
		m_description(description)
{
}

ParametersGroup::ParametersGroup(ParametersGroup&& pg) :
		m_optionsDescr(std::move(pg.m_optionsDescr)),
		m_optionsDescrWithGroup(std::move(pg.m_optionsDescrWithGroup)),
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
}

void ParametersGroup::readPOVarsMap(const boost::program_options::variables_map& clOpts)
{
	for (auto &it : m_parameters)
	{
		it.second->getFromPO(clOpts, m_groupName + ".");
	}
}

const boost::program_options::options_description& ParametersGroup::getOptionsDesctiption(bool groupsNeeded, bool defaultsNeeded)
{
	if (groupsNeeded)
	{
		m_optionsDescrWithGroup.reset(new boost::program_options::options_description(m_groupName.c_str()));
		for (auto &it : m_parameters)
			it.second->addToPO(*m_optionsDescrWithGroup, m_groupName + ".", defaultsNeeded);
		return *m_optionsDescrWithGroup;
	} else {
		m_optionsDescr.reset(new boost::program_options::options_description(m_groupName.c_str()));
		for (auto &it : m_parameters)
			it.second->addToPO(*m_optionsDescr, "", defaultsNeeded);
		return *m_optionsDescr;
	}
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
		m_title(title)
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
}

void Parameters::parseCmdline(int argc, const char* const * argv, bool useFull, bool useShort)
{
	m_vm.clear();
	rebuildOptionsDescriptions();
	namespace po = boost::program_options;
	try
	{
		if (useFull && useShort)
			po::store(po::parse_command_line(argc, argv, *m_clOptionsBoth), m_vm);
		else if (useFull)
			po::store(po::parse_command_line(argc, argv, *m_clOptionsWithGroups), m_vm);
		else if (useShort)
			po::store(po::parse_command_line(argc, argv, *m_clOptions), m_vm);
		else
			throw std::runtime_error("Parsing cmdline impossible: at least one of useFull, useShort should be true");
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

void Parameters::cmdlineHelp(std::ostream& stream, bool printFullForm)
{
	rebuildOptionsDescriptions(true);
	if (printFullForm)
		stream << *m_clOptionsWithGroups;
	else
		stream << *m_clOptions;
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

ParametersGroup* Parameters::group(const std::string& groupName)
{
	auto it = m_groups.find(groupName);
	if (it == m_groups.end())
		return nullptr;
	return it->second;
}

ParametersGroup& Parameters::operator[](const std::string& groupName)
{
	return *group(groupName);
}

void Parameters::rebuildOptionsDescriptions(bool defaultsNeeded)
{
	m_clOptions.reset(new boost::program_options::options_description(m_title.c_str()));
	m_clOptionsWithGroups.reset(new boost::program_options::options_description(m_title.c_str()));
	m_clOptionsBoth.reset(new boost::program_options::options_description());
	for (auto &it : m_groups)
	{
		m_clOptions->add(it.second->getOptionsDesctiption(false, defaultsNeeded));
		m_clOptionsWithGroups->add(it.second->getOptionsDesctiption(true, defaultsNeeded));

		m_clOptionsBoth->add(it.second->getOptionsDesctiption(false, defaultsNeeded));
		m_clOptionsBoth->add(it.second->getOptionsDesctiption(true, defaultsNeeded));
	}
}

void PreconfiguredOperations::addGeneralOptions(Parameters& p, const std::string& group, bool help, bool saveIni, bool loadIni)
{
	ParametersGroup* g = p.group(group);
	if (g == nullptr)
	{
		p.addGroup(ParametersGroup(group.c_str()));
		g = p.group(group);
	}
	if (help)
		g->add(Parameter<bool>("help", "Print help", ParamterType::cmdLine));
	if (saveIni)
		g->add(Parameter<std::string>("ini-save", "Save setting to ini file", ParamterType::cmdLine));
	if (loadIni)
		g->add(Parameter<std::string>("ini-load", "Load setting from ini file", ParamterType::cmdLine));
}

bool PreconfiguredOperations::quickReadConfiguration(
		Parameters& p,
		const std::vector<std::string>& configFiles,
		int argc, const char * const * argv,
		const std::string& group
)
{
	// Reading command line to detect help and ini options
	p.parseCmdline(argc, argv, true, true);
	if (p[group.c_str()].get<bool>("help"))
	{
		p[group.c_str()].getInterface("help").markNotInitialized();
		p.cmdlineHelp(std::cout, true);
		return false;
	}

	// Reading first from configuration files
	for (auto it = configFiles.begin(); it != configFiles.end(); ++it)
	{
		if (!SystemUtils::probeFile(SystemUtils::replaceTilta(*it)))
			continue;

		p.parseIni(it->c_str());
	}

	auto &g = p[group.c_str()];

	// Reading last ini file from cmdline
	if (g.initialized("ini-load"))
	{
		p.parseIni(g.get<std::string>("ini-load").c_str());
	}

	// Reading than command line
	p.parseCmdline(argc, argv, true, true);

	if (g.initialized("ini-save"))
	{
		p.writeIni(g.get<std::string>("ini-save").c_str());
	}
	return true;
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

bool SystemUtils::probeFile(const std::string& file)
{
	return boost::filesystem::exists(file);
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
