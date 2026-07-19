struct ValueMod {
    float value = 1.0;
    std::optional<float> min;
    std::optional<float> max;
    bool resetOnMin = false;
    bool resetOnMax = false;
};

struct FormFilter {
    std::unordered_set<int> formTypes;
    TESForm* formFilter = nullptr;
    bool excludeFilter = false;
};

ValueMod ParseValueMod(const nlohmann::json_abi_v3_12_0::json& item) {
    ValueMod mod;
    if (item.contains("mod")) {
        auto& data = item.at("mod");
        if (data.contains("value")) mod.value = data.at("value").get<float>();
        if (data.contains("min")) mod.min = data.at("min").get<float>();
        if (data.contains("max")) mod.max = data.at("max").get<float>();
        if (data.contains("resetOnMin")) mod.resetOnMin = data.at("resetOnMin").get<bool>();
        if (data.contains("resetOnMax")) mod.resetOnMax = data.at("resetOnMax").get<bool>();
    }
    return mod;
}

std::optional<FormFilter> ParseFormFilter(const nlohmann::json_abi_v3_12_0::json& data) {
    FormFilter filter;
    if (data.contains("form")) {
        filter.formFilter = Utils::GetForm<TESForm>(data.at("form").get<std::string>());
        if (!filter.formFilter) return std::nullopt;
    }
    if (data.contains("type")) Utils::FillSet<int>(data.at("type"), filter.formTypes);
    if (data.contains("exclude")) filter.excludeFilter = data.at("exclude").get<bool>();
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

    return true;
}

bool ValidateConditionForm(TESForm* a_form) {
    switch (a_form->GetFormType()) {
        case FormType::Perk:
            return player->HasPerk(a_form->As<BGSPerk>());
        case FormType::MagicEffect:
            return player->HasMagicEffect(a_form->As<EffectSetting>());
        case FormType::Faction:
            return player->IsInFaction(a_form->As<TESFaction>());
        case FormType::Location:
            return Utils::MatchLocation(player->GetCurrentLocation(), a_form->As<BGSLocation>());
    }
    return false;
}

void UpdateGlobalValue(TESGlobal* global, ValueMod& mod, float fMult = 1.0f) {
    // ConsoleLog::GetSingleton()->Print(fmt::format("global: {}, modvalue: {} with mult: {}", clib_util::editorID::get_editorID(global), mod.value, fMult).c_str());
    if (mod.value == 0.0f || fMult == 0.0f) {
        global->value = 0;
    } else {
        float baseValue = mod.value * fMult;
        float newValue = global->value += baseValue;
        if (mod.max.has_value() && newValue >= mod.max.value()) {
            if (mod.resetOnMax)
                newValue = 0;
            else
                newValue = mod.max.value();
        } else if (mod.min.has_value() && newValue <= mod.min.value()) {
            if (mod.resetOnMin)
                newValue = 0;
            else
                newValue = mod.min.value();
        }
        global->value = newValue;
    }
}