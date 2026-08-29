#include "../../oracle/src/oracleimporter.h"

#include "gtest/gtest.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <libcockatrice/card/format/format_legality_rules.h>
#include <libcockatrice/card/set/card_set.h>
#include <libcockatrice/interfaces/noop_card_set_priority_controller.h>

class OracleImporterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        controller = new NoopCardSetPriorityController();
        importer = new OracleImporter();
        set = CardSet::newInstance(controller, "TST", "Test Set");
    }

    void TearDown() override
    {
        delete importer;
        delete controller;
    }

    // Helper: build a minimal card JSON object
    QJsonObject makeCard(const QString &name,
                         const QString &colors = "",
                         const QString &colorIdentity = "",
                         const QVariantMap &legalities = {})
    {
        QJsonObject card;
        card["name"] = name;
        card["text"] = "Rules text.";
        card["layout"] = "normal";
        card["manaCost"] = "{W}";
        card["type"] = "Creature — Human";
        card["types"] = QJsonArray{"Creature"};
        card["number"] = "1";
        card["rarity"] = "common";

        if (!colors.isEmpty()) {
            QJsonArray arr;
            for (const QChar &c : colors) {
                arr.append(QString(c));
            }
            card["colors"] = arr;
        }
        if (!colorIdentity.isEmpty()) {
            QJsonArray arr;
            for (const QChar &c : colorIdentity) {
                arr.append(QString(c));
            }
            card["colorIdentity"] = arr;
        }
        if (!legalities.isEmpty()) {
            QJsonObject legalObj;
            for (auto it = legalities.constBegin(); it != legalities.constEnd(); ++it) {
                legalObj[it.key()] = it.value().toString();
            }
            card["legalities"] = legalObj;
        }

        QJsonObject identifiers;
        identifiers["scryfallId"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        card["identifiers"] = identifiers;

        return card;
    }

    NoopCardSetPriorityController *controller;
    OracleImporter *importer;
    CardSetPtr set;
};

// ============================================================================
// sortAndReduceColors tests (tested via importCardsFromSet)
// ============================================================================

TEST_F(OracleImporterTest, SortAndReduceColorsSingleColor)
{
    QJsonArray cards{makeCard("Red Card", "R", "R")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Red Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("colors"), "R");
}

TEST_F(OracleImporterTest, SortAndReduceColorsDeduplicates)
{
    QJsonArray cards{makeCard("Dedup Card", "WWUUB", "WU")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Dedup Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("colors"), "WUB");
}

TEST_F(OracleImporterTest, SortAndReduceColorsSortsWUBRG)
{
    QJsonArray cards{makeCard("Sort Card", "RGW", "RGW")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Sort Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("colors"), "WRG");
}

TEST_F(OracleImporterTest, SortAndReduceColorsAllFive)
{
    QJsonArray cards{makeCard("Five Color", "BRGWU", "BRGWU")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Five Color");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("colors"), "WUBRG");
}

TEST_F(OracleImporterTest, SortAndReduceColorIdentity)
{
    QJsonArray cards{makeCard("Color Id Card", "W", "GWR")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Color Id Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("coloridentity"), "WRG");
}

TEST_F(OracleImporterTest, SingleColorNotSorted)
{
    QJsonArray cards{makeCard("Single Card", "B", "B")};
    importer->importCardsFromSet(set, cards);

    auto card = importer->getCardList().value("Single Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("colors"), "B");
}

// ============================================================================
// Legality guard tests
// ============================================================================

TEST_F(OracleImporterTest, LegalityGuardCombinesForNewCard)
{
    QVariantMap leg;
    leg["standard"] = "legal";
    leg["modern"] = "legal";
    QJsonArray cards{makeCard("Legal Card", "", "", leg)};

    importer->importCardsFromSet(set, cards);
    auto card = importer->getCardList().value("Legal Card");
    ASSERT_FALSE(card.isNull());
    ASSERT_EQ(card->getProperty("format-standard"), "legal");
    ASSERT_EQ(card->getProperty("format-modern"), "legal");
}

TEST_F(OracleImporterTest, LegalityGuardPreservesFirstPrinting)
{
    // First printing: standard=legal, modern=legal
    QVariantMap leg1;
    leg1["standard"] = "legal";
    leg1["modern"] = "legal";
    QJsonArray cards1{makeCard("Guarded Card", "", "", leg1)};
    importer->importCardsFromSet(set, cards1);

    // Second printing: standard=banned, modern=not_legal
    CardSetPtr set2 = CardSet::newInstance(controller, "TS2", "Second Set");
    QVariantMap leg2;
    leg2["standard"] = "banned";
    leg2["modern"] = "not_legal";
    QJsonArray cards2{makeCard("Guarded Card", "", "", leg2)};
    importer->importCardsFromSet(set2, cards2);

    auto card = importer->getCardList().value("Guarded Card");
    ASSERT_FALSE(card.isNull());
    // Guard should preserve first printing's legalities
    ASSERT_EQ(card->getProperty("format-standard"), "legal");
    ASSERT_EQ(card->getProperty("format-modern"), "legal");
}

// ============================================================================
// createDefaultMagicFormats tests
// ============================================================================

TEST_F(OracleImporterTest, CreateDefaultMagicFormatsContainsExpectedFormats)
{
    auto formats = importer->createDefaultMagicFormats();
    ASSERT_TRUE(formats.contains("standard"));
    ASSERT_TRUE(formats.contains("modern"));
    ASSERT_TRUE(formats.contains("legacy"));
    ASSERT_TRUE(formats.contains("vintage"));
    ASSERT_TRUE(formats.contains("commander"));
    ASSERT_TRUE(formats.contains("pauper"));
    ASSERT_TRUE(formats.contains("pioneer"));
    ASSERT_TRUE(formats.contains("brawl"));
    ASSERT_TRUE(formats.contains("historic"));
    ASSERT_TRUE(formats.contains("timeless"));
    ASSERT_TRUE(formats.contains("duel"));
    ASSERT_TRUE(formats.contains("oathbreaker"));
}

TEST_F(OracleImporterTest, CreateDefaultMagicFormatsSingletonDeckSizes)
{
    auto formats = importer->createDefaultMagicFormats();
    auto commander = formats.value("commander");
    ASSERT_EQ(commander->minDeckSize, 100);
    ASSERT_EQ(commander->maxDeckSize, 100);
    ASSERT_EQ(commander->maxSideboardSize, 15);

    auto brawl = formats.value("brawl");
    ASSERT_EQ(brawl->minDeckSize, 60);
    ASSERT_EQ(brawl->maxDeckSize, 60);
}

TEST_F(OracleImporterTest, CreateDefaultMagicFormatsVintageHasRestricted)
{
    auto formats = importer->createDefaultMagicFormats();
    auto vintage = formats.value("vintage");
    bool hasRestricted = false;
    for (const auto &ac : vintage->allowedCounts) {
        if (ac.label == "restricted") {
            hasRestricted = true;
            ASSERT_EQ(ac.max, 1);
        }
    }
    ASSERT_TRUE(hasRestricted);
}

TEST_F(OracleImporterTest, CreateDefaultMagicFormatsRegexMatchesBasicLands)
{
    auto formats = importer->createDefaultMagicFormats();
    auto standard = formats.value("standard");
    ASSERT_FALSE(standard->exceptions.isEmpty());

    auto &basicLandsException = standard->exceptions.first();
    ASSERT_FALSE(basicLandsException.conditions.isEmpty());

    auto &condition = basicLandsException.conditions.first();
    ASSERT_EQ(condition.field, "type");
    ASSERT_EQ(condition.matchType, "regex");

    // Verify the regex actually works (was broken before: \b = backspace, not word boundary)
    QRegularExpression regex(condition.value);
    ASSERT_TRUE(regex.isValid());
    ASSERT_TRUE(regex.match("Basic Land — Forest").hasMatch());
    ASSERT_TRUE(regex.match("Basic Snow Land — Mountain").hasMatch());
    ASSERT_FALSE(regex.match("Creature — Elf Warrior").hasMatch());
}

TEST_F(OracleImporterTest, CreateDefaultMagicFormatsCaching)
{
    auto first = importer->createDefaultMagicFormats();
    auto second = importer->createDefaultMagicFormats();
    ASSERT_EQ(first.size(), second.size());
    ASSERT_EQ(first.keys(), second.keys());
}

// ============================================================================
// readSetsFromByteArray tests
// ============================================================================

TEST_F(OracleImporterTest, ReadSetsFromByteArrayValidJson)
{
    QJsonObject setObj;
    setObj["code"] = "tst";
    setObj["name"] = "Test Set";
    setObj["type"] = "expansion";
    setObj["releaseDate"] = "2024-01-01";
    setObj["cards"] = QJsonArray();

    QJsonObject root;
    root["data"] = QJsonObject{{"TST", setObj}};

    QByteArray data = QJsonDocument(root).toJson();
    ASSERT_TRUE(importer->readSetsFromByteArray(data));
    ASSERT_EQ(importer->getSets().size(), 1);
    ASSERT_EQ(importer->getSets().first().getShortName(), "TST");
}

TEST_F(OracleImporterTest, ReadSetsFromByteArrayInvalidJson)
{
    QByteArray data = "not valid json";
    ASSERT_FALSE(importer->readSetsFromByteArray(data));
}

TEST_F(OracleImporterTest, ReadSetsFromByteArrayEmptyData)
{
    QJsonObject root;
    root["data"] = QJsonObject();

    QByteArray data = QJsonDocument(root).toJson();
    ASSERT_FALSE(importer->readSetsFromByteArray(data));
}

TEST_F(OracleImporterTest, ReadSetsFromByteArrayCapitalizesSetType)
{
    QJsonObject setObj;
    setObj["code"] = "ftv";
    setObj["name"] = "From The Vault";
    setObj["type"] = "from_the_vault";
    setObj["releaseDate"] = "2024-01-01";
    setObj["cards"] = QJsonArray();

    QJsonObject root;
    root["data"] = QJsonObject{{"FTV", setObj}};

    QByteArray data = QJsonDocument(root).toJson();
    ASSERT_TRUE(importer->readSetsFromByteArray(data));
    ASSERT_EQ(importer->getSets().first().getSetType(), "From the Vault");
}

TEST_F(OracleImporterTest, ReadSetsFromByteArraySortsSetsByName)
{
    QJsonObject setA;
    setA["code"] = "zzz";
    setA["name"] = "Zeta Set";
    setA["type"] = "expansion";
    setA["releaseDate"] = "2024-01-01";
    setA["cards"] = QJsonArray();

    QJsonObject setB;
    setB["code"] = "aaa";
    setB["name"] = "Alpha Set";
    setB["type"] = "expansion";
    setB["releaseDate"] = "2024-01-01";
    setB["cards"] = QJsonArray();

    QJsonObject root;
    root["data"] = QJsonObject{{"ZZZ", setA}, {"AAA", setB}};

    QByteArray data = QJsonDocument(root).toJson();
    ASSERT_TRUE(importer->readSetsFromByteArray(data));
    auto sets = importer->getSets();
    ASSERT_GE(sets.size(), 2);
    ASSERT_EQ(sets.first().getShortName(), "AAA");
}

// ============================================================================
// Split card coloridentity tests
// ============================================================================

TEST_F(OracleImporterTest, SplitCardColorIdentityConcatenated)
{
    QJsonObject leg{{"standard", "not_legal"}};

    QJsonObject face1;
    face1["name"] = "Fire // Ice";
    face1["text"] = "Fire deals 2 damage.";
    face1["layout"] = "split";
    face1["side"] = "a";
    face1["faceName"] = "Fire";
    face1["colors"] = QJsonArray{"R"};
    face1["colorIdentity"] = QJsonArray{"R"};
    face1["types"] = QJsonArray{"Instant"};
    face1["manaCost"] = "{R}";
    face1["legalities"] = leg;
    face1["identifiers"] = QJsonObject{{"scryfallId", "aaa"}};
    face1["number"] = "1";
    face1["rarity"] = "uncommon";

    QJsonObject face2;
    face2["name"] = "Fire // Ice";
    face2["text"] = "Ice taps target artifact.";
    face2["layout"] = "split";
    face2["side"] = "b";
    face2["faceName"] = "Ice";
    face2["colors"] = QJsonArray{"U"};
    face2["colorIdentity"] = QJsonArray{"U"};
    face2["types"] = QJsonArray{"Instant"};
    face2["manaCost"] = "{U}";
    face2["legalities"] = leg;
    face2["identifiers"] = QJsonObject{{"scryfallId", "bbb"}};
    face2["number"] = "1";
    face2["rarity"] = "uncommon";

    QJsonArray cardsList{face1, face2};
    int count = importer->importCardsFromSet(set, cardsList);
    ASSERT_EQ(count, 1);

    auto card = importer->getCardList().value("Fire // Ice");
    ASSERT_FALSE(card.isNull());

    // coloridentity should be "RU" (concatenated), not "R // U"
    QString ci = card->getProperty("coloridentity");
    ASSERT_FALSE(ci.contains("//")) << "coloridentity should not contain '//', got: " << ci.toStdString();
    ASSERT_TRUE(ci.contains("R"));
    ASSERT_TRUE(ci.contains("U"));
}

TEST_F(OracleImporterTest, SplitCardColorsConcatenated)
{
    QJsonObject leg{{"standard", "not_legal"}};

    QJsonObject face1;
    face1["name"] = "Fire // Ice";
    face1["text"] = "Fire deals 2 damage.";
    face1["layout"] = "split";
    face1["side"] = "a";
    face1["faceName"] = "Fire";
    face1["colors"] = QJsonArray{"R"};
    face1["colorIdentity"] = QJsonArray{"R"};
    face1["types"] = QJsonArray{"Instant"};
    face1["manaCost"] = "{R}";
    face1["legalities"] = leg;
    face1["identifiers"] = QJsonObject{{"scryfallId", "aaa"}};
    face1["number"] = "1";
    face1["rarity"] = "uncommon";

    QJsonObject face2;
    face2["name"] = "Fire // Ice";
    face2["text"] = "Ice taps target artifact.";
    face2["layout"] = "split";
    face2["side"] = "b";
    face2["faceName"] = "Ice";
    face2["colors"] = QJsonArray{"U"};
    face2["colorIdentity"] = QJsonArray{"U"};
    face2["types"] = QJsonArray{"Instant"};
    face2["manaCost"] = "{U}";
    face2["legalities"] = leg;
    face2["identifiers"] = QJsonObject{{"scryfallId", "bbb"}};
    face2["number"] = "1";
    face2["rarity"] = "uncommon";

    QJsonArray cardsList{face1, face2};
    importer->importCardsFromSet(set, cardsList);

    auto card = importer->getCardList().value("Fire // Ice");
    ASSERT_FALSE(card.isNull());

    QString colors = card->getProperty("colors");
    ASSERT_FALSE(colors.contains("//")) << "colors should not contain '//', got: " << colors.toStdString();
    ASSERT_TRUE(colors.contains("R"));
    ASSERT_TRUE(colors.contains("U"));
}

// ============================================================================
// Mana cost formatting tests
// ============================================================================

TEST_F(OracleImporterTest, ManaCostStripsBraces)
{
    QJsonObject card = makeCard("Mana Card");
    card["manaCost"] = "{2}{W}{B}";
    QJsonArray cards{card};

    importer->importCardsFromSet(set, cards);
    auto result = importer->getCardList().value("Mana Card");
    ASSERT_FALSE(result.isNull());
    ASSERT_EQ(result->getProperty("manacost"), "2WB");
}

// ============================================================================
// Card deduplication tests
// ============================================================================

TEST_F(OracleImporterTest, DuplicateCardNameReturnsExisting)
{
    QJsonArray cards{makeCard("Dupe Card")};
    importer->importCardsFromSet(set, cards);

    CardSetPtr set2 = CardSet::newInstance(controller, "TS2", "Second Set");
    QJsonArray cards2{makeCard("Dupe Card")};
    importer->importCardsFromSet(set2, cards2);

    ASSERT_EQ(importer->getCardList().size(), 1);
}

TEST_F(OracleImporterTest, AELigatureReplaced)
{
    QJsonObject card = makeCard(QString::fromUtf8("\xC3\x86ther Vial")); // Æther Vial
    QJsonArray cards{card};

    importer->importCardsFromSet(set, cards);
    // Æ is replaced with AE, resulting in "AEther Vial"
    ASSERT_FALSE(importer->getCardList().contains(QString::fromUtf8("\xC3\x86ther Vial")));
    ASSERT_TRUE(importer->getCardList().contains("AEther Vial"));
}

TEST_F(OracleImporterTest, ApostropheNormalized)
{
    QJsonObject card = makeCard(QString::fromUtf8("Jace\u2019s Ingenuity"));
    QJsonArray cards{card};

    importer->importCardsFromSet(set, cards);
    ASSERT_TRUE(importer->getCardList().contains("Jace's Ingenuity"));
}

// ============================================================================
// RawJson scanner tests
// ============================================================================

TEST_F(OracleImporterTest, ScanSetRangesMatchFullJsonParse)
{
    QJsonObject root;
    QJsonObject data;
    data["AAA"] = makeCard("Alpha Card");
    data["BBB"] = makeCard("Beta Card");
    root["data"] = data;

    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);

    QList<RawJson::SetRange> ranges;
    const RawJson::ScanError error = RawJson::scanSetRanges(bytes, ranges);
    ASSERT_FALSE(error.isError()) << error.message.toStdString();
    ASSERT_EQ(ranges.size(), 2);

    const QJsonObject wholeData = QJsonDocument::fromJson(bytes).object().value("data").toObject();
    for (const RawJson::SetRange &range : ranges) {
        QJsonParseError parseError;
        const QJsonDocument sliceDoc =
            QJsonDocument::fromJson(QByteArray(bytes.constData() + range.start, range.length), &parseError);
        ASSERT_EQ(parseError.error, QJsonParseError::NoError)
            << range.code.toStdString() << ": " << parseError.errorString().toStdString();
        ASSERT_EQ(sliceDoc.object(), wholeData.value(range.code).toObject()) << "set " << range.code.toStdString();
    }
}

TEST_F(OracleImporterTest, ScanSetRangesDecodesEscapesAndCountsCards)
{
    const QByteArray json = "{\"data\":{\"KEY\":{\"code\":\"zzz\",\"name\":\"\\u00c9tude \\ud83d\\ude00\","
                            "\"type\":\"expansion\",\"releaseDate\":\"2024-01-05\","
                            "\"cards\":[{\"name\":\"a\"},{\"name\":\"b\"},{\"name\":\"c\"}]}}}";

    QList<RawJson::SetRange> ranges;
    ASSERT_FALSE(RawJson::scanSetRanges(json, ranges).isError());
    ASSERT_EQ(ranges.size(), 1);

    const RawJson::SetRange &range = ranges.first();
    ASSERT_EQ(range.code, "zzz"); // inner "code" wins over the object key
    const QString expectedName = QString::fromUtf8("\xC3\x89tude ") + QChar(0xD83D) + QChar(0xDE00);
    ASSERT_EQ(range.name, expectedName);
    ASSERT_EQ(range.type, "expansion");
    ASSERT_EQ(range.releaseDate, "2024-01-05");
    ASSERT_EQ(range.cardCount, 3);

    QJsonParseError parseError;
    const QJsonDocument sliceDoc =
        QJsonDocument::fromJson(QByteArray(json.constData() + range.start, range.length), &parseError);
    ASSERT_EQ(parseError.error, QJsonParseError::NoError);
    ASSERT_EQ(sliceDoc.object().value("name").toString(), expectedName);
    ASSERT_EQ(sliceDoc.object().value("cards").toArray().size(), 3);
}

TEST_F(OracleImporterTest, ScanSetRangesRejectsInvalidJson)
{
    const QList<QByteArray> invalid = {"not json",
                                       "[]",
                                       "{\"data\":[]}",
                                       "{\"data\":{}}",
                                       "{\"other\":{}}",
                                       "{\"data\":{\"A\":{\"code\":\"a\",\"name\":\"ok\",\"type\":\"x\","
                                       "\"releaseDate\":\"2024-01-01\",\"cards\":[]}}} trailing",
                                       "{\"data\":{\"A\":{\"cards\":[{\"name\":\"\\uZZZZ\"}]}}}",
                                       "{\"data\":{\"A\":{\"cards\":[{\"name\":\"bad \\q escape\"}]}}}",
                                       "{\"data\":{\"A\":{\"cards\":[{\"name\":\"\\ud800\"}]}}}"};

    for (const QByteArray &json : invalid) {
        QList<RawJson::SetRange> ranges;
        const RawJson::ScanError error = RawJson::scanSetRanges(json, ranges);
        EXPECT_TRUE(error.isError()) << "expected failure for: " << json.constData();
    }
}

// ============================================================================
// Lazy per-set parsing tests
// ============================================================================

TEST_F(OracleImporterTest, StartImportParsesSetsLazily)
{
    QJsonObject setObj = makeCard("Lazy Import Card");
    QJsonArray cards;
    cards.append(setObj);
    QJsonObject dataSet;
    dataSet["code"] = "tst";
    dataSet["name"] = "Test Set";
    dataSet["type"] = "expansion";
    dataSet["releaseDate"] = "2024-01-01";
    dataSet["cards"] = cards;

    QJsonObject root;
    root["data"] = QJsonObject{{"TST", dataSet}};

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    ASSERT_TRUE(importer->readSetsFromByteArray(data));
    ASSERT_FALSE(importer->getRawSetsData().isEmpty());

    const int importedSets = importer->startImport();
    ASSERT_EQ(importedSets, 1);
    ASSERT_EQ(importer->getCardList().size(), 1);
    ASSERT_FALSE(importer->getCardList().value("Lazy Import Card").isNull());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
