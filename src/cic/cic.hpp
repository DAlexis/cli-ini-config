/**
 * cli-ini-config library developed to maximally simplify
 * work with command line and ini-files options at the
 * same time
 *
 * @todo: Boolean parameters support in form --bool-parameter=false
 */

#ifndef LIBHEADER_INCLUDED
#define LIBHEADER_INCLUDED

#include "utils.hpp"
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <stdexcept>
#include <string>
#include <list>
#include <memory>
#include <iostream>

#include <initializer_list>

#define CIC_ASSERT(condition, message) if (not (condition)) throw std::runtime_error(std::string((message)));

namespace cic {

enum class ParamterType
{
	//disabled = 0,
	iniFile = 1,
	cmdLine = 2,
	both    = iniFile | cmdLine
};

class IAnyTypeParameter
{
public:
	virtual ~IAnyTypeParameter() {}
	virtual std::string toString() const = 0;
	virtual const std::string& name() const = 0;

	virtual void addToPO(boost::program_options::options_description& od, const std::string& prefix = "", bool defaultsNeeded = false) const = 0;
	virtual bool getFromPO(const boost::program_options::variables_map& clOpts, const std::string& optionalPrefix = "") = 0;
	virtual bool getFromPT(const boost::property_tree::ptree& pt) = 0;

	virtual void writeIniItem(std::ostream& stream) = 0;

	virtual bool initialized() const = 0;
	virtual bool setByUser() const = 0;

	virtual IAnyTypeParameter* copy() const = 0;
};

template <typename T>
struct Parameter : public IAnyTypeParameter
{
	Parameter(const char* name, const char* description, T initValue, ParamterType pt = ParamterType::both) :
		m_value(initValue),
		m_name(name),
		m_description(description),
		m_isInitialized(true),
		m_parType(pt)
	{ }

	Parameter(const char* name, const char* description, ParamterType pt = ParamterType::both) :
		m_name(name),
		m_description(description),
		m_isInitialized(false),
		m_parType(pt)
	{
		initNoDefault();
	}

	const T& get()
	{
		CIC_ASSERT(m_isInitialized, std::string("Parameter ") + m_name + " usage without initialization!");
		return m_value;
	}

	const std::string& name() const override { return m_name; }

	std::string toString() const override
	{
		return StringTool<T>::to_string(m_value);
	}

	bool initialized() const override {	return m_isInitialized; }

	void addToPO(boost::program_options::options_description& od, const std::string& prefix = "", bool defaultsNeeded = false) const override;

	bool getFromPO(const boost::program_options::variables_map& clOpts, const std::string& optionalPrefix = "") override;

	bool getFromPT(const boost::property_tree::ptree& pt)
	{
		if (m_parType == ParamterType::iniFile || m_parType == ParamterType::both)
		{
			if (pt.count(m_name.c_str()) != 0)
			{
				m_value = pt.get<T>(m_name.c_str());
				m_setByUser = true;
				m_isInitialized = true;
			}
		}

		return initialized();
	}

	void writeIniItem(std::ostream& stream) override
	{
		if (m_parType != ParamterType::iniFile && m_parType != ParamterType::both)
			return;
		stream << "# " << m_description << std::endl;
		stream << m_name << " = ";
		if (m_isInitialized)
			stream << m_value << std::endl;
		else
			stream << "<value>" << std::endl;
	}

	bool setByUser() const override { return m_setByUser; }

private:
	virtual IAnyTypeParameter* copy() const override
	{
		Parameter *p = new Parameter(*this);
		return p;
	}

	/// Function to be easy overrided for bool parameter
	void initNoDefault();

	T m_value;
	std::string m_name;
	std::string m_description;
	bool m_isInitialized;
	bool m_setByUser = false;
	ParamterType m_parType = ParamterType::both;
};

template<typename T>
void Parameter<T>::addToPO(boost::program_options::options_description& od, const std::string& prefix, bool defaultsNeeded) const
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		if (m_isInitialized && defaultsNeeded)
		{
			od.add_options()
				((prefix + m_name).c_str(), boost::program_options::value<T>()->default_value(m_value), m_description.c_str());
		} else {
			od.add_options()
				((prefix + m_name).c_str(), boost::program_options::value<T>(), m_description.c_str());
		}
	}
}

template<typename T>
bool Parameter<T>::getFromPO(const boost::program_options::variables_map& clOpts, const std::string& optionalPrefix)
{
	if (m_parType == ParamterType::cmdLine || m_parType == ParamterType::both)
	{
		auto tryName = [this, &clOpts](const std::string& name) -> bool {
			if (clOpts.count(name.c_str()) == 0)
			{
				return false;
			}

			m_value = clOpts[name.c_str()].as<T>();
			m_setByUser = true;
			m_isInitialized = true;
			return true;
		};

		if (!tryName(optionalPrefix + m_name))
			tryName(m_name);
	}
	return initialized();
}

template<typename T>
void Parameter<T>::initNoDefault() { }

template<>
void Parameter<bool>::addToPO(boost::program_options::options_description& od, const std::string& prefix, bool defaultsNeeded) const;

template<>
bool Parameter<bool>::getFromPO(const boost::program_options::variables_map& clOpts, const std::string& optionalPrefix);

template<>
void Parameter<bool>::initNoDefault();

class ParametersGroup
{
public:
	ParametersGroup(const char* groupName, const char* description = "");

	template <typename... Args>
	ParametersGroup(const char* groupName, const char* description, Args... args) :
		m_groupName(groupName),
		m_description(description)
	{
		add(args...);
	}

	template <typename... Args>
	ParametersGroup(const char* groupName, Args... args) :
		m_groupName(groupName)
	{
		add(args...);
	}

	ParametersGroup(ParametersGroup&& pg);
	const std::string& name();

	template <typename... Args>
	void add(const IAnyTypeParameter& parameter, Args... args)
	{
		add(parameter);
		add(args...);
	}
	void add(const IAnyTypeParameter& parameter);

	void readPOVarsMap(const boost::program_options::variables_map& clOpts);

	const boost::program_options::options_description& getOptionsDesctiption(bool groupsNeeded, bool defaultsNeeded = false);

	/**
	 * Read variables from boost::property_tree
	 * returns false if at least one parameter in this group is not initialized yet
	 */
	bool readPT(const boost::property_tree::ptree& pt);
	void writeIniItem(std::ostream& stream);

	IAnyTypeParameter& getInterface(const std::string& name);

	template <typename T>
	const T& get(const std::string& name)
	{
		return dynamic_cast<Parameter<T>&>(getInterface(name)).get();
	}

	bool initialized(const std::string& name)
	{
		return getInterface(name).initialized();
	}

private:
	std::unique_ptr<boost::program_options::options_description> m_optionsDescr;
	std::unique_ptr<boost::program_options::options_description> m_optionsDescrWithGroup;

	bool areAllInitialized();
	std::string m_groupName;
	std::string m_description;
	std::map<std::string, std::unique_ptr<IAnyTypeParameter>> m_parameters;
};

class Parameters
{
public:
	Parameters(const char* title = "Allowed options");

	template <typename... Args>
	Parameters(const char* title, Args&&... args) :
		m_title(title)
	{
		addGroup(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void addGroup(ParametersGroup&& parameter, Args&&... args)
	{
		addGroup(std::move(parameter));
		addGroup(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void addGroup(ParametersGroup& parameter, Args&&... args)
	{
		addGroup(parameter);
		addGroup(std::forward<Args>(args)...);
	}

	void addGroup(ParametersGroup&& pg);
	void addGroup(ParametersGroup& pg);

	void parseCmdline(int argc, const char * const * argv, bool useFull = true, bool useShort = true);
	void parseIni(const char* filename);
	void parseIni(const std::vector<std::string>& variants, const std::string& suffix = "");

	void cmdlineHelp(std::ostream& stream, bool printFullForm = false);
	void writeIni(std::ostream& stream);
	void writeIni(const char* filename);

	const boost::program_options::variables_map& variablesMap();
	const boost::property_tree::ptree& propertyTree();

	ParametersGroup& operator[](const std::string& groupName);

private:
	void rebuildOptionsDescriptions(bool defaultsNeeded = false);

	std::string m_title;
	std::list<std::unique_ptr<ParametersGroup>> m_pgOwners;
	std::map<std::string, ParametersGroup*> m_groups;
	std::unique_ptr<boost::program_options::options_description> m_clOptions;
	std::unique_ptr<boost::program_options::options_description> m_clOptionsWithGroups;
	std::unique_ptr<boost::program_options::options_description> m_clOptionsBoth;
	boost::program_options::variables_map m_vm;
	boost::property_tree::ptree m_pt;
};

class SystemUtils
{
public:
	static std::string homeDir();
	static std::string replaceTilta(const std::string& source);
	static std::string probeFiles(const std::vector<std::string>& variants, const std::string& suffix = "");
};

} // namespace cic

#endif // LIBHEADER_INCLUDED
