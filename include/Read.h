#pragma once

namespace S_Read {
    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        std::optional<bool> skillBook;
        std::optional<bool> spellBook;
    };

    std::vector<Rule> globals;

    void Process(TESObjectBOOK* book, bool skillBook) {
        for (auto& item : globals) {
            if (item.skillBook.has_value() && skillBook != item.skillBook.value()) continue;
            if (item.spellBook.has_value() && book->TeachesSpell() != item.spellBook.value()) continue;
            if (item.formFilter && !Utils::ParseFormFilter(book, item.formFilter)) continue;

            item.global->value += 1;
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
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
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
}