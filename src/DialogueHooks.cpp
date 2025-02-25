#include "MiscHooks.h"
#include "DialogueHooks.h"
#include "Config.h"
#include "Utils.h"
#include "Manager.h"

namespace Hook
{
	struct DialogueResponseHook //INFO NAM1
	{
		static RE::DialogueResponse* thunk(RE::DialogueResponse* a_item, RE::TESTopic* a_topic, RE::TESTopicInfo* a_topicInfo, RE::TESObjectREFR* a_speaker, RE::TESTopicInfo::ResponseData* a_response)
		{
			auto result = func(a_item, a_topic, a_topicInfo, a_speaker, a_response);

			const std::string fileName = Utils::getModName(a_topicInfo);
			const RE::FormID trimmedFormID = Utils::getTrimmedFormID(a_topicInfo);

			if (Config::enableDebugLog)
			{
				std::stringstream ss;
				ss << "Original string: " << a_response->responseText.c_str()
					<< " - FormID: " << std::format("{0:08X}", a_topicInfo->formID)
					<< " - TrimmedFormID: " << std::format("{0:08X}", trimmedFormID)
					<< " - Number: " << static_cast<int>(a_response->responseNumber)
					<< " - Plugin: " << fileName;

				SKSE::log::debug("{}", ss.str());
			}

			Manager::GetSingleton()->getINFO_NAM1(trimmedFormID, fileName, a_response->responseNumber, a_item->text);

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(34413, 35220), 0xE4 };
			stl::write_thunk_call<DialogueResponseHook>(target1.address());

			REL::Relocation<std::uintptr_t> target2{ REL::VariantID(34436, 35256, 0x574360), 0xCD };
			stl::write_thunk_call<DialogueResponseHook>(target2.address());

			if (REL::Module::IsAE())
			{
				REL::Relocation<std::uintptr_t> target3{ REL::ID(35254), 0x2ED };
				stl::write_thunk_call<DialogueResponseHook>(target3.address());
			}
		}
	};

	struct SharedResponseDataHook // Hacky workaround for records with DNAM (shared response data)
	{
		struct MoveParent : Xbyak::CodeGenerator
		{
			MoveParent() // move RE::TESTopicInfo* as 3rd parameter
			{
				mov(r8, r12);
			}
		};

		static bool thunk(RE::TESFile* a_file, void* a_buf, std::uint64_t a_topicInfo)
		{
			auto result = func(a_file, a_buf, 4u); // invoke with 4u from original code

			const RE::TESTopicInfo* parentInfo = reinterpret_cast<RE::TESTopicInfo*>(a_topicInfo);
			std::uint32_t dnamFormID = *static_cast<std::uint32_t*>(a_buf);

			const auto lookupFile = Utils::getFileByFormIDRaw(dnamFormID, a_file);
			if (lookupFile == nullptr)
			{
				SKSE::log::error("INFO DNAM error, please report it in our Discord!");
				SKSE::log::error("FormID - Parent: {0:08X}", parentInfo->formID);
				SKSE::log::error("FormID - DNAM: {0:08X}", dnamFormID);
				SKSE::log::error("File - Parent: {}", a_file->GetFilename());
				return result;
			}

			RE::FormID formID = dnamFormID & 0xFFFFFF;
			if (lookupFile->IsLight())
			{
				formID &= 0xFFF;
			}

			auto& map = Manager::GetSingleton()->getINFO_NAM1_Map();
			const auto it = map.find(Manager::constructKey(formID, lookupFile->GetFilename().data()));
			if (it != map.end())
			{
				const std::string newKey = Manager::constructKey(Utils::getTrimmedFormID(parentInfo), Utils::getModName(parentInfo));
				map.insert({ newKey, it->second });
			}

			return result;
		}

		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			constexpr auto targetAddress = REL::VariantID(25058, 25597, 0x3A1310);

			REL::Relocation<std::uintptr_t> swapTarget{ targetAddress, REL::Relocate(0x332, 0x39F) };
			REL::safe_fill(swapTarget.address(), REL::NOP, 0x6);

			auto newCode = MoveParent();
			assert(newCode.getSize() <= 0x6);
			REL::safe_write(swapTarget.address(), newCode.getCode(), newCode.getSize());

			REL::Relocation<std::uintptr_t> target{ targetAddress, REL::Relocate(0x33F, 0x3AC) };
			stl::write_thunk_call<SharedResponseDataHook>(target.address());
		}
	};

	struct DialogueMenuTextHook //DIAL FULL, INFO RNAM
	{
		static void thunk(RE::MenuTopicManager::Dialogue& a_out, char* a2, std::uint64_t a3) //Skyrim is not only passing BSStrings into this function
		{
			func(a_out, a2, a3);

			const auto manager = Manager::GetSingleton();
			manager->getDIAL(Utils::getTrimmedFormID(a_out.parentTopic), Utils::getModName(a_out.parentTopic), a_out.topicText);
			manager->getDIAL(Utils::getTrimmedFormID(a_out.parentTopicInfo), Utils::getModName(a_out.parentTopicInfo), a_out.topicText); //INFO RNAM always overwrites DIAL FULL of parenttopic

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(34434, 35254), REL::Relocate(0xCC, 0x226) };
			stl::write_thunk_call<DialogueMenuTextHook>(target1.address());

			if (REL::Module::IsAE())
			{
				REL::Relocation<std::uintptr_t> target2{ REL::ID(35254), 0x115 };
				stl::write_thunk_call<DialogueMenuTextHook>(target2.address());
			}
		}
	};

	void InstallDialogueHooksPostLoad()
	{
		SharedResponseDataHook::Install();
	}

	void InstallDialogueHooks()
	{
		DialogueResponseHook::Install();
		DialogueMenuTextHook::Install();
	}

}