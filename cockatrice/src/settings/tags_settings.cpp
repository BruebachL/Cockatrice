#include "tags_settings.h"

inline QStringList DEFAULT_TAGS = {
    // Strategies
    "🏃️ Aggro",
    "🧙‍️ Control",
    "⚔️ Midrange",
    "🌀 Combo",
    "🪓 Mill",
    "🔒 Stax",
    "🗺️ Landfall",
    "🛡️ Pillowfort",
    "🌱 Ramp",
    "⚡ Storm",
    "💀 Aristocrats",
    "☠️ Reanimator",
    "👹 Sacrifice",
    "🔥 Burn",
    "🌟 Lifegain",
    "🔮 Spellslinger",
    "👥 Tokens",
    "🎭 Blink",
    "⏳ Time Manipulation",
    "🌍 Domain",
    "💫 Proliferate",
    "📜 Saga",
    "🎲 Chaos",
    "🪄 Auras",
    "🔫 Pingers",

    // Themes
    "👑 Monarch",
    "🚀 Vehicles",
    "💉 Infect",
    "🩸 Madness",
    "🌀 Morph",

    // Card Types
    "⚔️ Creature",
    "💎 Artifact",
    "🌔 Enchantment",
    "📖 Sorcery",
    "⚡ Instant",
    "🌌 Planeswalker",
    "🌏 Land",
    "🪄 Aura",

    // Kindred Types
    "🐉 Kindred",
    "🧙 Humans",
    "⚔️ Soldiers",
    "🛡️ Knights",
    "🎻 Bards",
    "🧝 Elves",
    "🌲 Dryads",
    "😇 Angels",
    "🎩 Wizards",
    "🧛 Vampires",
    "🦴 Skeletons",
    "💀 Zombies",
    "👹 Demons",
    "👾 Eldrazi",
    "🐉 Dragons",
    "🐠 Merfolk",
    "🦁 Cats",
    "🐺 Wolves",
    "🐺 Werewolves",
    "🦇 Bats",
    "🐀 Rats",
    "🦅 Birds",
    "🦗 Insects",
    "🍄 Fungus",
    "🐚 Sea Creatures",
    "🐗 Boars",
    "🦊 Foxes",
    "🦄 Unicorns",
    "🐘 Elephants",
    "🐻 Bears",
    "🦏 Rhinos",
    "🦂 Scorpions",
};

TagsSettings::TagsSettings(const QString &settingPath, QObject *parent)
    : SettingsManager(settingPath + "tags.ini", parent)
{
}

QStringList TagsSettings::getDefaultTags()
{
    auto value = getValue("tags", "defaults");
    if (value.isNull()) {
        return DEFAULT_TAGS;
    }
    return value.toStringList();
}

void TagsSettings::setDefaultTags(const QStringList &tags)
{
    setValue(tags, "tags", "defaults");
    emit defaultTagsChanged();
}
