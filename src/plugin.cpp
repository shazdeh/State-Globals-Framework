#include "logger.h"
#include <unordered_set>
#include "Utils.h"
#include "SpellLearn.h"
#include "Equipment.h"
#include "Kills.h"
#include "MagicEffect.h"
#include "Inventory.h"
#include "Magic.h"

//#include "ClibUtil/editorID.hpp"

static void ParseData(const json& data) {
    for (const auto& item : data) {
        if (!item.contains("global")) continue;

        TESGlobal* global = Utils::GetForm<TESGlobal>(item.at("global").get<std::string>());
        if (!global) continue;

        if (item.contains("learnspell")) {
            auto result = S_SpellLearn::parseJSON(item);
            if (result.has_value()) {
                S_SpellLearn::globals.insert({global, result.value()});
            }
        }
        if (item.contains("equip")) {
            auto result = Equipment::parseJSON(item);
            if (result.has_value()) {
                Equipment::globals.insert({global, result.value()});
            }
        }
        if (item.contains("kill")) {
            auto result = Kills::parseJSON(item);
            if (result.has_value()) {
                Kills::globals.insert({global, result.value()});
            }
        }
        if (item.contains("magiceffect")) {
            auto result = S_MagicEffect::parseJSON(item);
            if (result.has_value()) {
                S_MagicEffect::globals.insert({global, result.value()});
            }
        }
        if (item.contains("inventory")) {
            auto result = S_Inventory::parseJSON(item);
            if (result.has_value()) {
                S_Inventory::globals.insert({global, result.value()});
            }
        }
        if (item.contains("spellcast")) {
            auto result = S_SpellCast::parseJSON(item);
            if (result.has_value()) {
                S_SpellCast::globals.insert({global, result.value()});
            }
        }
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

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        BuildRules();
        // ConsoleLog::GetSingleton()->Print(fmt::format("Size of kills map: {}", Kills::globals.size()).c_str());
        Equipment::SetupEvents();
        Kills::SetupEvents();
        S_SpellLearn::SetupEvents();
        S_MagicEffect::SetupEvents();
        S_Inventory::SetupEvents();
        S_SpellCast::SetupEvents();
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
