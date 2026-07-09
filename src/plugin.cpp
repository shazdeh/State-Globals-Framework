#include "logger.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "Utils.h"
#include <unordered_set>
// #include "SpellLearn.h"
#include "Equipment.h"
#include "Kills.h"

//#include "ClibUtil/editorID.hpp"

// std::unordered_map<TESGlobal*, SpellLearn::Rule> spellLearnGlobals;

static void ParseData(const json& data) {
    for (const auto& item : data) {
        if (!item.contains("global")) continue;

        TESGlobal* global = Utils::GetForm<TESGlobal>(item.at("global").get<std::string>());
        if (!global) continue;

        if (item.contains("learnspell")) {
            // if (auto rule = LearnSpell::parseJSON(item); rule) {
            //     spellLearnGlobals.insert({global, rule});
            // }
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
