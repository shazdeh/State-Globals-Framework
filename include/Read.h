#pragma once

namespace S_Read {
    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
        std::optional<bool> skillBook;
        std::optional<bool> spellBook;
    };

    std::vector<Rule> globals;

    void Process(TESObjectBOOK* book, bool skillBook) {
        for (auto& item : globals) {
            if (item.skillBook.has_value() && skillBook != item.skillBook.value()) continue;
            if (item.spellBook.has_value() && book->TeachesSpell() != item.spellBook.value()) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(book, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<BooksRead::Event> {
        BSEventNotifyControl ProcessEvent(const BooksRead::Event* event, BSTEventSource<BooksRead::Event>*) {
            if (!event || !event->book) return BSEventNotifyControl::kContinue;
            Process(event->book, event->skillBook);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("read")) return;
        auto& data = item.at("read");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("skillBook")) {
            rule.skillBook = data.at("skillBook").get<bool>();
        }
        if (data.contains("spellBook")) {
            rule.spellBook = data.at("spellBook").get<bool>();
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink theSink;
            BooksRead::GetEventSource()->AddEventSink(&theSink);
        }
    }

    void OnLoadGame() {
        auto& all = TESDataHandler::GetSingleton()->GetFormArray<TESObjectBOOK>();
        for (auto book : all) {
            Process(book, book->TeachesSkill());
        }
    }
}