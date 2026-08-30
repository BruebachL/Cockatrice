#include <QRegularExpression>
#include <QScopedPointer>
#include <gtest/gtest.h>
#include <metrics_registry.h>

TEST(MetricsRegistryTest, EmptyRegistryProducesNoHistogramLines)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);

    EXPECT_EQ(0, registry->totalCommands());
    EXPECT_EQ(0, registry->totalTimeMs());
    EXPECT_EQ(0, registry->activeTypeCount());

    const QString text = registry->toPrometheusText([](int) { return QString("x"); }, {});
    // The family TYPE declaration may stand alone; what must not exist is a
    // histogram sample without data behind it.
    EXPECT_FALSE(text.contains(QRegularExpression("servatrice_commands_duration_ms_(bucket|sum|count)")));
}

TEST(MetricsRegistryTest, SingleSampleIsRecordedInTotalsAndBuckets)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeCommand(MetricsRegistry::typeIdFor(0, 1000), 7);

    EXPECT_EQ(1, registry->totalCommands());
    EXPECT_EQ(7, registry->totalTimeMs());
    EXPECT_EQ(1, registry->activeTypeCount());

    const QString text = registry->toPrometheusText(
        [](int typeId) {
            return QString("%1/%2").arg(typeId / MetricsRegistry::KindStride).arg(typeId % MetricsRegistry::KindStride);
        },
        {});
    // 7ms falls into the le="10" bucket; smaller buckets stay empty
    EXPECT_TRUE(text.contains("# TYPE servatrice_commands_duration_ms histogram\n"));
    EXPECT_TRUE(text.contains(",le=\"10\"} 1"));
    EXPECT_TRUE(text.contains(",le=\"5\"} 0"));
    EXPECT_TRUE(text.contains("_sum{command=\"0/1000\"} 7"));
    EXPECT_TRUE(text.contains("_count{command=\"0/1000\"} 1"));
}

TEST(MetricsRegistryTest, BucketsAreCumulative)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeCommand(MetricsRegistry::typeIdFor(0, 1000), 2);
    registry->observeCommand(MetricsRegistry::typeIdFor(0, 1000), 30);

    const QString text = registry->toPrometheusText([](int) { return QString("cmd"); }, {});

    // cumulative counts: <=25 -> 1 sample, <=50 -> 2 samples
    EXPECT_TRUE(text.contains(",le=\"25\"} 1\n"));
    EXPECT_TRUE(text.contains(",le=\"50\"} 2\n"));
    EXPECT_TRUE(text.contains(",le=\"+Inf\"} 2\n"));
}

TEST(MetricsRegistryTest, KindEncodingSeparatesSameExtensionNumber)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    const int sessionPing = MetricsRegistry::typeIdFor(0, 1000);
    const int roomLeaveRoom = MetricsRegistry::typeIdFor(1, 1000);
    ASSERT_NE(sessionPing, roomLeaveRoom);

    registry->observeCommand(sessionPing, 1);
    registry->observeCommand(roomLeaveRoom, 5000);

    EXPECT_EQ(2, registry->activeTypeCount());
}

TEST(MetricsRegistryTest, OutOfRangeIdsLandInOverflowSlot)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeCommand(-1, 4);
    registry->observeCommand(MetricsRegistry::MaxTypes + 12345, 4);

    EXPECT_EQ(2, registry->totalCommands());
    EXPECT_EQ(1, registry->activeTypeCount()); // both collapsed into one slot
    EXPECT_EQ(8, registry->totalTimeMs());
}

TEST(MetricsRegistryTest, NegativeDurationsAreClamped)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeCommand(MetricsRegistry::typeIdFor(0, 1000), -50);

    EXPECT_EQ(0, registry->totalTimeMs());
}

TEST(MetricsRegistryTest, GaugesAndLabelEscapingAreRendered)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);

    QHash<QString, qint64> gauges;
    gauges.insert("servatrice_users_current", 42);

    const QString text = registry->toPrometheusText(nullptr, gauges);
    EXPECT_TRUE(text.contains("# TYPE servatrice_users_current gauge\n"));
    EXPECT_TRUE(text.contains("servatrice_users_current 42\n"));
}

TEST(MetricsRegistryTest, UnnamedTypesFallBackToNumericLabel)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeCommand(MetricsRegistry::typeIdFor(2, 1042), 9);

    const QString text = registry->toPrometheusText(nullptr, {});
    EXPECT_TRUE(text.contains("{command=\"" + QString::number(MetricsRegistry::typeIdFor(2, 1042)) + "\"}"));
}

TEST(MetricsRegistryTest, GameStartHistogramOnlyAppearsAfterSamples)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    EXPECT_FALSE(registry->toPrometheusText(nullptr, {}).contains("servatrice_game_start_duration_ms"));

    registry->observeGameStartDurationMs(120);
    const QString text = registry->toPrometheusText(nullptr, {});
    // 120ms falls into the le="250" bucket
    EXPECT_TRUE(text.contains("# TYPE servatrice_game_start_duration_ms histogram\n"));
    EXPECT_TRUE(text.contains(",le=\"100\"} 0\n"));
    EXPECT_TRUE(text.contains(",le=\"250\"} 1\n"));
    EXPECT_TRUE(text.contains("servatrice_game_start_duration_ms_sum 120\n"));
    EXPECT_TRUE(text.contains("servatrice_game_start_duration_ms_count 1\n"));
}

TEST(MetricsRegistryTest, GameStartHistogramIsSeparateFromCommandTotals)
{
    QScopedPointer<MetricsRegistry> registry(new MetricsRegistry);
    registry->observeGameStartDurationMs(10);

    EXPECT_EQ(0, registry->totalCommands());
    EXPECT_EQ(0, registry->totalTimeMs());
    EXPECT_EQ(0, registry->activeTypeCount());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
