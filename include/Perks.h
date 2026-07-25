#pragma once

namespace S_Perks {
    struct Rule {
        TESGlobal* global;
        std::optional<FormFilter> formFilter;
        bool owned = true;
        std::unordered_set<ActorValue> skills;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

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
            for (int i = 6; i < 24; i++) {
                ActorValue av = static_cast<ActorValue>(i);
                if (!item.skills.empty() && !item.skills.contains(av)) continue;
                for (auto perk : map.at(av)) {
                    if (item.formFilter.has_value() && !ValidateFormFilter(perk, item.formFilter.value())) continue;
                    if (item.owned == player->HasPerk(perk)) value++;
                }
            }
            UpdateGlobalValue(item.global, item.mod, value, true);
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
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("skill")) {
            Utils::FillSet(data.at("skill"), rule.skills);
        }
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            UI::GetSingleton()->AddEventSink<MenuOpenCloseEvent>(&g_sink);
        }
    }

    void OnLoadGame() { Process();
    }
}