struct ValueMod {
    float base = 0.0f;
    float value = 1.0;
    std::optional<float> min;
    std::optional<float> max;
    bool resetOnMin = false;
    bool resetOnMax = false;
};

struct FormFilter {
    std::unordered_set<int> formTypes;
    TESForm* formFilter = nullptr;
    std::vector<BGSKeyword*> magicEffectKeyword;
    bool excludeFilter = false;
    bool keywordMatchAll = false;
};

struct ConditionFilter {
    TESForm* form = nullptr;
    std::vector<BGSKeyword*> locationKeyword;
    float value = 0.0;
    Compare compare = Compare::Equal;
};

ValueMod ParseValueMod(const nlohmann::json_abi_v3_12_0::json& item) {
    ValueMod mod;
    if (item.contains("mod")) {
        auto& data = item.at("mod");
        if (data.is_number()) {
            mod.value = data.get<float>();
        } else {
            if (data.contains("value")) mod.value = data.at("value").get<float>();
            if (data.contains("min")) mod.min = data.at("min").get<float>();
            if (data.contains("max")) mod.max = data.at("max").get<float>();
            if (data.contains("resetOnMin")) mod.resetOnMin = data.at("resetOnMin").get<bool>();
            if (data.contains("resetOnMax")) mod.resetOnMax = data.at("resetOnMax").get<bool>();
            if (data.contains("base")) mod.base = data.at("base").get<float>();
        }
    }
    return mod;
}

std::optional<FormFilter> ParseFormFilter(const nlohmann::json_abi_v3_12_0::json& data) {
    FormFilter filter;
    if (data.is_string()) {
        filter.formFilter = Utils::GetForm<TESForm>(data.get<std::string>());
        if (!filter.formFilter) return std::nullopt;
    } else {
        if (data.contains("form")) {
            filter.formFilter = Utils::GetForm<TESForm>(data.at("form").get<std::string>());
            if (!filter.formFilter) return std::nullopt;
        }
        if (data.contains("magicEffectKeyword")) {
            if (!Utils::FillFormsArray<BGSKeyword>(data.at("magicEffectKeyword"), filter.magicEffectKeyword))
                return std::nullopt;
        }
        if (data.contains("type")) Utils::FillSet<int>(data.at("type"), filter.formTypes);
        if (data.contains("exclude")) filter.excludeFilter = data.at("exclude").get<bool>();
    }
    if (!filter.formFilter && filter.magicEffectKeyword.empty() && filter.formTypes.empty()) {
        return std::nullopt;
    }
    return filter;
}

std::optional<ConditionFilter> ParseConditionFilter(const nlohmann::json_abi_v3_12_0::json& data) {
    ConditionFilter filter;
    if (data.is_string()) {
        filter.form = Utils::GetForm<TESForm>(data.get<std::string>());
        if (!filter.form) return std::nullopt;
    } else {
        if (data.contains("form")) {
            filter.form = Utils::GetForm<TESForm>(data.at("form").get<std::string>());
            if (!filter.form) return std::nullopt;
            if (data.contains("value")) {
                filter.value = data.at("value").get<float>();
            }
            if (data.contains("compare")) {
                filter.compare = Utils::ParseCompareOperator(data.at("compare").get<std::string>());
            }
        }
        if (data.contains("locationKeyword")) {
            if (!Utils::FillFormsArray<BGSKeyword>(data.at("locationKeyword"), filter.locationKeyword))
                return std::nullopt;
        }
    }

    if (!filter.form && filter.locationKeyword.empty()) {
        return std::nullopt;
    }

    return filter;
}

bool ValidateFormFilter(TESForm* a_form, FormFilter& a_filter) {
    if (!a_filter.formTypes.empty() && !a_filter.formTypes.contains(std::to_underlying(a_form->GetFormType())))
        return false;
    if (a_filter.formFilter) {
        bool isMatching = a_form->IsActor() ? Utils::ParseActorFilter(a_form->As<Actor>(), a_filter.formFilter)
                                            : Utils::ParseFormFilter(a_form, a_filter.formFilter);
        if (a_filter.excludeFilter == isMatching) {
            return false;
        }
    }

    if (!a_filter.magicEffectKeyword.empty() && !Utils::FormHasMagicEffectKeyword(a_form, a_filter.magicEffectKeyword))
        return false;

    return true;
}

bool ValidateConditionForm(ConditionFilter& filter) {
    if (filter.form) {
        switch (filter.form->GetFormType()) {
            case FormType::Perk:
                if (!player->HasPerk(filter.form->As<BGSPerk>())) return false;
                break;
            case FormType::MagicEffect:
                if (!player->HasMagicEffect(filter.form->As<EffectSetting>())) return false;
                break;
            case FormType::Faction:
                if (Utils::DoCompare(player->GetFactionRank(filter.form->As<TESFaction>(), true),
                                     static_cast<int>(filter.value), filter.compare))
                    return false;
                break;
            case FormType::Location:
                if (!Utils::MatchLocation(player->GetCurrentLocation(), filter.form->As<BGSLocation>())) return false;
                break;
            case FormType::Global:
                if (!Utils::DoCompare(filter.form->As<TESGlobal>()->value, filter.value, filter.compare)) return false;
                break;
            case FormType::Quest:
                if (!Utils::DoCompare(filter.form->As<TESQuest>()->GetCurrentStageID(),
                                      static_cast<uint16_t>(filter.value), filter.compare))
                    return false;
                break;
        }
    }

    if (!filter.locationKeyword.empty() &&
        !player->GetCurrentLocation()->HasKeywordInArray(filter.locationKeyword, false)) {
        return false;
    }

    return true;
}

void UpdateGlobalValue(TESGlobal* global, ValueMod& mod, float fMult = 1.0f, bool bOverride = false) {
    float oldValue = global->value;
    float newValue = oldValue;
    bool reset = false;

    if (mod.value == 0.0f || fMult == 0.0f) {
        newValue = 0;
    } else {
        newValue = (bOverride ? 0.0f : oldValue) + (mod.value * fMult);
        if (mod.max.has_value() && newValue >= mod.max.value()) {
            if (mod.resetOnMax)
                reset = true;
            newValue = mod.max.value();
        } else if (mod.min.has_value() && newValue <= mod.min.value()) {
            if (mod.resetOnMin)
                reset = true;
            newValue = mod.min.value();
        }
    }

    // run actions before value reset
    if (newValue != oldValue) {
        S_Action::RunActions(global, newValue);
    }

    if (reset) {
        newValue = mod.base;
    }
    global->value = newValue;
}