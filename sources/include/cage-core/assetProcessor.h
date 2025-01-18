#ifndef guard_assetProcessor_h_fzj4lsea9874t8h74mjk
#define guard_assetProcessor_h_fzj4lsea9874t8h74mjk

#include <cage-core/assetHeader.h>

namespace cage
{
	class CAGE_CORE_API AssetProcessor : private Immovable
	{
	public:
		// provided names
		String inputDirectory; // c:/asset
		String inputName; // path/file?specifier;identifier
		String outputDirectory; // c:/data
		uint32 outputName = 0; // 123456789
		uint32 schemeIndex = 0;

		// derived names
		String inputFileName; // c:/asset/path/file
		String outputFileName; // c:/data/123456789
		String inputFile; // path/file
		String inputSpec; // specifier
		String inputIdentifier; // identifier

		void loadProperties();
		void derivedProperties();
		void logProperties() const;
		String property(const String &name) const;

		static String readLine();
		static void writeLine(const String &other);
		static void initializeSecondaryLog(const String &path);

		// relative path is interpreted relative to the input file (unless specified otherwise)
		// absolute path is interpreted as relative to input root path
		String convertAssetPath(const String &input, const String &relativeTo = "", bool markAsReferenced = true) const;
		String convertFilePath(const String &input, const String &relativeTo = "", bool markAsUsed = true) const;

		AssetHeader initializeAssetHeader() const;
	};

	CAGE_CORE_API Holder<AssetProcessor> newAssetProcessor();
}

#endif // guard_assetProcessor_h_fzj4lsea9874t8h74mjk
