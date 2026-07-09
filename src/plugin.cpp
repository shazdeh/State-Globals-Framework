#include "logger.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "Utils.h"
#include <unordered_set>
#include "SpellLearn.h"
#include "Equipment.h"
#include "Kills.h"

//#include "ClibUtil/editorID.hpp"


#pragma once

namespace S_MagicEffect {
    bool bQueued;

    struct Rule {
        std::optional<MagicSystem::SpellType> spellType;
        bool unique = true;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void Process() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        float value = 0.0f;
        auto effects = mt->GetActiveEffectList();
        for (auto& item : globals) {
            std::unordered_set<MagicItem*> visited;
            for (auto* effect : *effects) {
                if (!effect || !effect->spell) continue;
                if (item.second.spellType && effect->spell->GetSpellType() != item.second.spellType) continue;
                if (item.second.unique) {
                    if (visited.contains(effect->spell)) continue;
                    visited.insert(effect->spell);
                }
                
                value += 1;
            }
            item.first->value = value;
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESMagicEffectApplyEvent> {
        BSEventNotifyControl ProcessEvent(const TESMagicEffectApplyEvent* event,
                                          BSTEventSource<TESMagicEffectApplyEvent>*) {
            if (bQueued || !event || !event->target || !event->target->IsPlayerRef())
                return BSEventNotifyControl::kContinue;
            SKSE::GetTaskInterface()->AddTask(Process);
            bQueued = true;
            return BSEventNotifyControl::kContinue;
        }
    };

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("magiceffect");
        Rule rule;
        if (data.contains("spellType")) {
            rule.spellType = static_cast<MagicSystem::SpellType>(data.at("spellType").get<int>());
        }
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESMagicEffectApplyEvent>(&g_sink);
        }
    }
}

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
