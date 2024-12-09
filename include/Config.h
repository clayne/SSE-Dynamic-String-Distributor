#pragma once
#include <glaze/glaze.hpp>

class Config : public ISingleton<Config>
{
public:
	void enumerateFolder();
	void parseTranslationFiles();
	void onDataLoad();

	enum class SubrecordType
	{
		kFULL, //Used for const translation
		kSHRT,
		kTNAM,
		kDATA,
		kRDMP,
		kDESC,
		kCNAM,
		kDNAM,
		kNNAM,
		kITXT,
		kEPFD,

		kUnknown
	};

	static inline bool enableDebugLog = false;

private:
	template <class T>
	struct custom_nullable_t
	{
		std::optional<T> val{};
		bool has_value() const { return val.has_value(); }

		T& value() { return *val; }
		const T& value() const { return *val; }

		template <class... Args>
		void emplace(Args&&... args) {
			val.emplace(std::forward<Args>(args)...);
		}
	};

	struct Data
	{
		std::string form_id{};
		std::string type{};
		std::string string{};
		std::string original{};
		custom_nullable_t<int> index{};
		custom_nullable_t<std::string> editor_id{};
	};

	std::vector<std::string> getLoadOrder();
	std::vector<std::string> enumerateFilesInFolders(const std::string& folders);
	std::string getSubrecordType(const std::string& types) const;
	std::tuple<RE::FormID, std::string> extractFormIDAndPlugin(const std::string& formIDWithPlugin);
	RE::FormID convertToFormID(std::string input);
	SubrecordType getSubrecordType_map(std::string_view type);
	void processEntry(Data& entry, const std::string& file);

	std::vector<std::string> m_folders;
	std::vector<std::string> m_loadOrder;
	std::vector<std::string> m_filesInPluginFolder;

	std::vector<Data> m_onDataLoad;
};