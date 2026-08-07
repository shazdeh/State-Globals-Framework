#include "logger.h"
#undef GetObject
#include <unordered_set>
#include "Utils.h"
#include "SimpleIni.h"

SpellItem* LastHitSpell;
PlayerCharacter* player;
TESObjectWEAP* Unarmed;
bool bLogIDs = false;

void RunActions(TESGlobal* global, float a_globalValue);

#include "Base.h"
#include "Action.h"
#include "SpellLearn.h"
#include "Equip.h"
#include "Kills.h"
#include "ActiveEffect.h"
#include "Inventory.h"
#include "Magic.h"
#include "Hits.h"
#include "Combat.h"
#include "Perks.h"
#include "Barter.h"
#include "Pickpocket.h"
#include "RefCount.h"
#include "Read.h"
#include "Location.h"
#include "Save.h"
#include "SoulTrap.h"
#include "Sleep.h"
#include "Wait.h"
#include "Ini.h"
#include "ModEvent.h"
#include "Craft.h"
// #include "Quest.h"

// what a mess
void RunActions(TESGlobal* global, float a_globalValue) {
    S_Action::RunActions(global, a_globalValue);
}

static void ParseData(const json& data) {
    for (const auto& item : data) {
        if (!item.contains("global")) continue;

        TESGlobal* global = Utils::GetForm<TESGlobal>(item.at("global").get<std::string>());
        if (!global) continue;

        S_Action::parseJSON(item, global);
        S_Magic::parseJSON(item, global);
        S_Inventory::parseJSON(item, global);
        S_ActiveEffect::parseJSON(item, global);
        S_Equip::parseJSON(item, global);
        S_Kills::parseJSON(item, global);
        S_SpellLearn::parseJSON(item, global);
        S_Combat::parseJSON(item, global);
        S_Perks::parseJSON(item, global);
        S_Hits::parseJSON(item, global);
        S_Barter::parseJSON(item, global);
        S_RefCount::parseJSON(item, global);
        S_Read::parseJSON(item, global);
        S_Pickpocket::parseJSON(item, global);
        S_Location::parseJSON(item, global);
        S_Save::parseJSON(item, global);
        S_SoulTrap::parseJSON(item, global);
        S_Sleep::parseJSON(item, global);
        S_Wait::parseJSON(item, global);
        S_Ini::parseJSON(item, global);
        S_ModEvent::parseJSON(item, global);
        S_Craft::parseJSON(item, global);
        //S_Quest::parseJSON(item, global);
    }
}

static void BuildRules() {
    const std::filesystem::path dir = "Data/SKSE/Plugins/State Globals";
    if (!std::filesystem::exists(dir)) return;
    for (auto& file : std::filesystem::directory_iterator(dir)) {
        std::ifstream ifile{file.path()};
        if (!ifile) continue;
        try {
            json data = json::parse(ifile);
            if (data.is_discarded()) continue;
            ParseData(data);
        } catch (...) {
            logger::warn("Error in parsing JSON file {}", file.path().string());
        }
    }
}

bool LoadConfig() {
    CSimpleIniA ini;
    if (ini.LoadFile("Data/SKSE/Plugins/StateGlobalsFramework.ini") == SI_OK) {
        LastHitSpell = Utils::GetForm<SpellItem>(ini.GetValue("Forms", "LastHitSpell", ""));
        if (!LastHitSpell) return false;
        Unarmed = Utils::GetForm<TESObjectWEAP>(ini.GetValue("Forms", "Unarmed", ""));
        bLogIDs = ini.GetBoolValue("Debug", "LogIDs", false);
    } else {
        return false;
    }

    return true;
}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        player = PlayerCharacter::GetSingleton();
        LoadConfig();
        BuildRules();
        S_Equip::SetupEvents();
        S_Kills::SetupEvents();
        S_SpellLearn::SetupEvents();
        S_ActiveEffect::SetupEvents();
        S_Inventory::SetupEvents();
        S_Magic::SetupEvents();
        S_Combat::SetupEvents();
        S_Perks::SetupEvents();
        S_Hits::SetupEvents();
        S_Barter::SetupEvents();
        S_RefCount::SetupEvents();
        S_Read::SetupEvents();
        S_Pickpocket::SetupEvents();
        S_Location::SetupEvents();
        S_SoulTrap::SetupEvents();
        S_Sleep::SetupEvents();
        S_Wait::SetupEvents();
        S_ModEvent::SetupEvents();
        S_Craft::SetupEvents();
        //S_Quest::SetupEvents();
    } else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        S_Save::Process();
    } else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // adjust variables for newly loaded game
        S_Equip::OnLoadGame();
        S_Inventory::OnLoadGame();
        S_Perks::OnLoadGame();
        S_SpellLearn::OnLoadGame();
        S_Ini::OnLoadGame();

        S_Barter::OnLoadGame();
        S_Craft::OnLoadGame();
        S_Hits::OnLoadGame();
        S_Kills::OnLoadGame();
        S_Magic::OnLoadGame();
        S_Read::OnLoadGame();
        S_SoulTrap::OnLoadGame();
    } else if (message->type == SKSE::MessagingInterface::kNewGame) {
        S_Ini::OnLoadGame();
    }
}

bool PapyrusBinder(BSScript::IVirtualMachine* vm) {
    std::string_view script = "StateGlobals";

    vm->RegisterFunction("GetLastVendor", script, S_Barter::GetLastVendor);
    vm->RegisterFunction("GetLastCraftedObject", script, S_Craft::GetLastCraftedObject);
    vm->RegisterFunction("GetLastUsedWorkbench", script, S_Craft::GetLastUsedWorkbench);
    vm->RegisterFunction("GetLastHitTarget", script, S_Hits::GetLastHitTarget);
    vm->RegisterFunction("GetLastHitTakenTarget", script, S_Hits::GetLastHitTakenTarget);
    vm->RegisterFunction("GetLastKilledActor", script, S_Kills::GetLastKilledActor);
    vm->RegisterFunction("GetLastCastedSpell", script, S_Magic::GetLastCastedSpell);
    vm->RegisterFunction("GetLastBookRead", script, S_Read::GetLastBookRead);
    vm->RegisterFunction("GetLastSoulTrappedActor", script, S_SoulTrap::GetLastSoulTrappedActor);

    return false;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    SKSE::GetPapyrusInterface()->Register(PapyrusBinder);
    return true;
}
