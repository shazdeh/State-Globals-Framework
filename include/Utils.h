#pragma once

#include "CLibUtilsQTR/FormReader.hpp"

namespace Utils {
    template <typename T>
    T* GetForm(const std::string& a_id) {
        auto id = FormReader::GetFormEditorIDFromString(a_id);
        if (id) {
            return TESForm::LookupByID<T>(id);
        }
        return nullptr;
    }

    template <typename T>
    bool compare(T a, T b, const std::string& op) {
        if (op == "=" || op == "==") return a == b;
        if (op == "<") return a < b;
        if (op == "<=") return a <= b;
        if (op == ">") return a > b;
        if (op == ">=") return a >= b;
        if (op == "!=") return a != b;
        return false;
    }

    template <typename T>
    void FillSet(const nlohmann::json_abi_v3_12_0::json& data, std::unordered_set<T>& a_set) {
        if (data.is_array()) {
            for (auto& item : data) {
                a_set.insert(item.get<T>());
            }
        } else {
            a_set.insert(data.get<T>());
        }
    }

    template <typename T>
    bool fillFormsArray(const nlohmann::json_abi_v3_12_0::json& data, std::vector<T*>& arr) {
        if (data.is_array()) {
            for (auto& item : data) {
                T* form = GetForm<T>(item.get<std::string>());
                if (form) arr.push_back(form);
                else return false;
            }
        } else if (data.is_string()) {
            T* form = GetForm<T>(data.get<std::string>());
            if (form) arr.push_back(form);
            else return false;
        }

        return true;
    }

    template <typename T>
    bool FillFormsSet(const nlohmann::json_abi_v3_12_0::json& data, std::unordered_set<T*>& a_set) {
        if (data.is_array()) {
            for (auto& item : data) {
                T* form = GetForm<T>(item.get<std::string>());
                if (form)
                    a_set.insert(form);
                else
                    return false;
            }
        } else if (data.is_string()) {
            T* form = GetForm<T>(data.get<std::string>());
            if (form)
                a_set.insert(form);
            else
                return false;
        }

        return true;
    }

    BSTArray<RE::Effect*> GetMagicEffects(TESForm* a_form) {
        switch (a_form->GetFormType()) {
            case FormType::Spell:
                return a_form->As<SpellItem>()->effects;

            case FormType::AlchemyItem:
                return a_form->As<AlchemyItem>()->effects;

            case FormType::Ingredient:
                return a_form->As<IngredientItem>()->effects;

            case FormType::Scroll:
                return a_form->As<ScrollItem>()->effects;

            case FormType::Enchantment:
                return a_form->As<EnchantmentItem>()->effects;
        }
        return {};
    }

    bool FormHasAnyMagicEffect(TESForm* a_form, std::unordered_set<EffectSetting*> a_effectsSet) {
        BSTArray<RE::Effect*> effects = GetMagicEffects(a_form);

        if (effects.size()) {
            for (Effect* effect : effects) {
                if (a_effectsSet.contains(effect->baseEffect)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool FormHasMagicEffectKeyword(TESForm* a_form, std::vector<BGSKeyword*> a_keywords) {
        BSTArray<RE::Effect*> effects = GetMagicEffects(a_form);
        if (effects.size()) {
            for (Effect* effect : effects) {
                if (effect->baseEffect->HasKeywordInArray(a_keywords, false)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool ParseFormFilter(TESForm* a_form, TESForm* a_filter) {
        if (a_filter == a_form) return true;
        if (a_filter->GetFormType() == FormType::FormList) {
            auto list = a_filter->As<BGSListForm>();
            if (list->ContainsOnlyType(FormType::Keyword)) {
                return a_form->HasKeywordInList(list, false);
            } else {
                return list->HasForm(a_form);
            }
        } else if (a_filter->GetFormType() == FormType::Keyword) {
            auto keyword = a_filter->As<BGSKeyword>();
            return a_form->HasKeywordInArray({keyword}, true);
        }

        return false;
    }

    bool MatchLocation(BGSLocation* current, BGSLocation* location) {
        while (current) {
            if (current == location) return true;
            current = current->parentLoc;
        }
        return false;
    }

    bool ActorIsInAnyFactionInList(Actor* a_actor, BGSListForm* a_list) {
        bool is = false;
        a_list->ForEachForm([a_actor, &is](TESForm* form) {
            auto faction = form->As<TESFaction>();
            if (faction && a_actor->IsInFaction(faction)) {
                is = true;
                return BSContainer::ForEachResult::kStop;
            }
            return BSContainer::ForEachResult::kContinue;
        });
        return is;
    }

    bool ActorIsInAnyLocationInList(Actor* a_actor, BGSListForm* a_list) {
        bool is = false;
        auto currentLocation = a_actor->GetCurrentLocation();
        a_list->ForEachForm([currentLocation, &is](TESForm* form) {
            auto location = form->As<BGSLocation>();
            if (location && MatchLocation(currentLocation, location)) {
                is = true;
                return BSContainer::ForEachResult::kStop;
            }
            return BSContainer::ForEachResult::kContinue;
        });
        return is;
    }

    // a_filter can be: TESActor, BGSKeyword, TESFaction, BGSLocation
    // or a FormList containing a set of one of those
    bool ParseActorFilter(Actor* a_actor, TESForm* a_filter) {
        if (a_actor == a_filter) return true;
        auto type = a_filter->GetFormType();
        switch (type) {
            case FormType::FormList: {
                auto list = a_filter->As<BGSListForm>();
                if (list->ContainsOnlyType(FormType::Keyword)) {
                    return a_actor->HasKeywordInList(list, false);
                } else if (list->ContainsOnlyType(FormType::Faction)) {
                    return ActorIsInAnyFactionInList(a_actor, list);
                } else if (list->ContainsOnlyType(FormType::Location)) {
                    return ActorIsInAnyLocationInList(a_actor, list);
                } else {
                    return list->HasForm(a_actor);
                }
            }
            case FormType::Keyword: {
                auto keyword = a_filter->As<BGSKeyword>();
                return a_actor->HasKeywordInArray({keyword}, true);
            }
            case FormType::Faction: {
                auto faction = a_filter->As<TESFaction>();
                return a_actor->IsInFaction(faction);
            }
            case FormType::Location: {
                auto currentLocation = a_actor->GetCurrentLocation();
                auto location = a_filter->As<BGSLocation>();
                return MatchLocation(currentLocation, location);
            }
        }

        return false;
    }
}