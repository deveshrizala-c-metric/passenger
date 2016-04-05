/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2010-2016 Phusion Holding B.V.
 *
 *  "Passenger", "Phusion Passenger" and "Union Station" are registered
 *  trademarks of Phusion Holding B.V.
 *
 *  See LICENSE file for license information.
 */
#ifndef _PASSENGER_RESOURCE_LOCATOR_H_
#define _PASSENGER_RESOURCE_LOCATOR_H_

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <pwd.h>
#include <Constants.h>
#include <Exceptions.h>
#include <Utils.h>
#include <Utils/IniFile.h>

namespace Passenger {

using namespace std;
using namespace boost;


/**
 * Locates various Phusion Passenger resources on the filesystem. All Phusion Passenger
 * files are located through this class. There's similar code in src/ruby_supportlib/phusion_passenger.rb.
 * See doc/Packaging.txt.md for an introduction about where Phusion Passenger expects its
 * files to be located.
 */
class ResourceLocator {
private:
	string installSpec;
	string binDir;
	string supportBinariesDir;
	string helperScriptsDir;
	string resourcesDir;
	string docDir;
	string rubyLibDir;
	string nodeLibDir;
	string buildSystemDir;

	static string getOption(const string &file, const IniFileSectionPtr &section, const string &key) {
		if (section->hasKey(key)) {
			return section->get(key);
		} else {
			throw RuntimeException("Option '" + key + "' missing in file '" + file + "'");
		}
	}

	static string getOptionalSection(const string &file, const IniFileSectionPtr &section, const string &key) {
		if (section->hasKey(key)) {
			return section->get(key);
		} else {
			return string();
		}
	}

public:
	ResourceLocator() { }

	ResourceLocator(const string &_installSpec)
		: installSpec(_installSpec)
	{
		if (getFileType(_installSpec) == FT_REGULAR) {
			const string &file = _installSpec;
			IniFileSectionPtr options = IniFile(file).section("locations");
			binDir              = getOption(file, options, "bin_dir");
			supportBinariesDir  = getOption(file, options, "support_binaries_dir");
			helperScriptsDir    = getOption(file, options, "helper_scripts_dir");
			resourcesDir        = getOption(file, options, "resources_dir");
			docDir              = getOption(file, options, "doc_dir");
			rubyLibDir          = getOption(file, options, "ruby_libdir");
			nodeLibDir          = getOption(file, options, "node_libdir");
			buildSystemDir      = getOptionalSection(file, options, "node_libdir");
		} else {
			const string &root = _installSpec;
			binDir              = root + "/bin";
			supportBinariesDir  = root + "/buildout/support-binaries";
			helperScriptsDir    = root + "/src/helper-scripts";
			resourcesDir        = root + "/resources";
			docDir              = root + "/doc";
			rubyLibDir          = root + "/src/ruby_supportlib";
			nodeLibDir          = root + "/src/nodejs_supportlib";
			buildSystemDir      = root;
		}
	}

	const string &getInstallSpec() const {
		return installSpec;
	}

	const string &getBinDir() const {
		return binDir;
	}

	const string &getSupportBinariesDir() const {
		return supportBinariesDir;
	}

	string getUserSupportBinariesDir() const {
		struct passwd pwd, *user;
		long bufSize;
		shared_array<char> strings;

		// _SC_GETPW_R_SIZE_MAX is not a maximum:
		// http://tomlee.co/2012/10/problems-with-large-linux-unix-groups-and-getgrgid_r-getgrnam_r/
		bufSize = std::max<long>(1024 * 128, sysconf(_SC_GETPW_R_SIZE_MAX));
		strings.reset(new char[bufSize]);

		user = (struct passwd *) NULL;
		if (getpwuid_r(getuid(), &pwd, strings.get(), bufSize, &user) != 0) {
			user = (struct passwd *) NULL;
		}

		if (user == (struct passwd *) NULL) {
			int e = errno;
			throw SystemException("Cannot lookup system user database", e);
		}

		string result(user->pw_dir);
		result.append("/");
		result.append(USER_NAMESPACE_DIRNAME);
		result.append("/support-binaries/");
		result.append(PASSENGER_VERSION);
		return result;
	}

	const string &getHelperScriptsDir() const {
		return helperScriptsDir;
	}

	const string &getResourcesDir() const {
		return resourcesDir;
	}

	const string &getDocDir() const {
		return docDir;
	}

	const string &getRubyLibDir() const {
		return rubyLibDir;
	}

	const string &getNodeLibDir() const {
		return nodeLibDir;
	}

	// Can be empty.
	const string &getBuildSystemDir() const {
		return buildSystemDir;
	}

	string findSupportBinary(const string &name) {
		string path = getSupportBinariesDir() + "/" + name;
		bool found;
		try {
			found = fileExists(path);
		} catch (const SystemException &e) {
			found = false;
		}
		if (found) {
			return path;
		}

		path = getUserSupportBinariesDir() + "/" + name;
		if (fileExists(path)) {
			return path;
		}

		throw RuntimeException("Support binary " + name + " not found (tried: "
			+ getSupportBinariesDir() + "/" + name + " and " + path + ")");
	}
};

typedef boost::shared_ptr<ResourceLocator> ResourceLocatorPtr;


}

#endif /* _PASSENGER_RESOURCE_LOCATOR_H_ */
