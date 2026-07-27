#include "logger.h"
#undef GetObject
#include <unordered_set>
#include "Utils.h"
#include "SimpleIni.h"

SpellItem* LastHitSpell;
PlayerCharacter* player;
TESObjectWEAP* Unarmed;
bool bLogIDs = false;

#include "Base.h"
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

static void ParseData(const json& data) {
    for (const auto& item : data) {
        if (!item.contains("global")) continue;

        TESGlobal* global = Utils::GetForm<TESGlobal>(item.at("global").get<std::string>());
        if (!global) continue;

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
    } else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        S_Save::Process();
    } else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        S_Equip::OnLoadGame();
        S_Inventory::OnLoadGame();
        S_Perks::OnLoadGame();
        S_Read::OnLoadGame();
        S_SpellLearn::OnLoadGame();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
