#pragma once

namespace S_Perks {
    struct Rule {
        bool owned = true;
        std::unordered_set<ActorValue> skills;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void LogTreeInternal(BGSSkillPerkTreeNode* node, std::unordered_set<BGSSkillPerkTreeNode*>& visited,
                         std::unordered_set<BGSPerk*>& perks) {
        if (!node || visited.contains(node)) return;
        visited.insert(node);
        for (BGSPerk* p = node->perk; p; p = p->nextPerk) {  // handle perks with multiple ranks
            perks.emplace(p);
        }
        for (auto child : node->children) LogTreeInternal(child, visited, perks);
    }

    std::unordered_set<BGSPerk*> GetAllPerks(BGSSkillPerkTreeNode* root) {
        std::unordered_set<BGSSkillPerkTreeNode*> visited;
        std::unordered_set<BGSPerk*> perks;
        if (root) LogTreeInternal(root, visited, perks);
        return perks;
    }

    void Process() {
        auto avList = ActorValueList::GetSingleton();
        if (!avList) return;
        auto player = PlayerCharacter::GetSingleton();
        static std::map<ActorValue, std::unordered_set<BGSPerk*>> map;
        static bool init = false;
        if (!init) {
            for (int i = 6; i < 24; i++) {  // from One-handed to Enchanting
                ActorValue av = static_cast<ActorValue>(i);
                auto root = avList->GetActorValueInfo(av)->perkTree;
                map.insert({av, GetAllPerks(root)});
            }
            init = true;
        }

        for (auto& item : globals) {
            float value = 0.0f;
            for (auto av : item.second.skills) {
                auto it = map.find(av);
                if (it != map.end()) {
                    for (auto perk : it->second) {
                        if (player->HasPerk(perk)) value++;
                    }
                }
            }
            item.first->value = value;
        }
    }

    class EventSink : public BSTEventSink<MenuOpenCloseEvent> {
        BSEventNotifyControl ProcessEvent(const MenuOpenCloseEvent* event, BSTEventSource<MenuOpenCloseEvent>*) {
            if (!event->opening && event->menuName == StatsMenu::MENU_NAME) {
                Process();
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("perk")) return;
        auto& data = item.at("perk");
        if (!data.contains("skill")) return;
        Rule rule;
        Utils::FillSet(data.at("skill"), rule.skills);
        globals.insert({global, rule});
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            UI::GetSingleton()->AddEventSink<MenuOpenCloseEvent>(&g_sink);
        }
    }
}