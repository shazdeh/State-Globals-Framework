#include "logger.h"
#undef GetObject
#include <unordered_set>
#include "Utils.h"

//BGSListForm* LastHitList;
SpellItem* LastHitSpell;

#include "Base.h"
#include "SpellLearn.h"
#include "Equip.h"
#include "Kills.h"
#include "MagicEffect.h"
#include "Inventory.h"
#include "Magic.h"
#include "Combat.h"
#include "Perks.h"
#include "Barter.h"
#include "Pickpocket.h"
#include "Proximity.h"
#include "Read.h"
#include "Hits.h"
#include "Location.h"




static void ParseData(const json& data) {
    for (const auto& item : data) {
        if (!item.contains("global")) continue;

        TESGlobal* global = Utils::GetForm<TESGlobal>(item.at("global").get<std::string>());
        if (!global) continue;

        S_Magic::parseJSON(item, global);
        S_Inventory::parseJSON(item, global);
        S_MagicEffect::parseJSON(item, global);
        S_Equip::parseJSON(item, global);
        S_Kills::parseJSON(item, global);
        S_SpellLearn::parseJSON(item, global);
        S_Combat::parseJSON(item, global);
        S_Perks::parseJSON(item, global);
        S_Hits::parseJSON(item, global);
        S_Barter::parseJSON(item, global);
        S_Proximity::parseJSON(item, global);
        S_Read::parseJSON(item, global);
        S_Pickpocket::parseJSON(item, global);
        S_Location::parseJSON(item, global);
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
        }
    }
}

bool LoadConfig() {
    std::ifstream ifile{"Data/SKSE/Plugins/StateGlobalsFramework.json"};
    if (!ifile) return false;
    try {
        json data = json::parse(ifile);
        if (data.is_discarded()) return false;
        //LastHitList = Utils::GetForm<BGSListForm>(data.at("LastHitList").get<std::string>());
        LastHitSpell = Utils::GetForm<SpellItem>(data.at("LastHit").get<std::string>());
        if (!LastHitSpell) return false;
    } catch (...) {
        return false;
    }
    return true;
}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        LoadConfig();
        BuildRules();
        // ConsoleLog::GetSingleton()->Print(fmt::format("Size of kills map: {}", Kills::globals.size()).c_str());
        S_Equip::SetupEvents();
        S_Kills::SetupEvents();
        S_SpellLearn::SetupEvents();
        S_MagicEffect::SetupEvents();
        S_Inventory::SetupEvents();
        S_Magic::SetupEvents();
        S_Combat::SetupEvents();
        S_Perks::SetupEvents();
        S_Hits::SetupEvents();
        S_Barter::SetupEvents();
        S_Proximity::SetupEvents();
        S_Read::SetupEvents();
        S_Pickpocket::SetupEvents();
        S_Location::SetupEvents();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame ||
        message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
