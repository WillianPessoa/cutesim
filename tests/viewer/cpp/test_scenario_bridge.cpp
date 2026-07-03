#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTemporaryDir>
#include "ScenarioBridge.h"

/* Tests ────────────────────────────────────────────────────────────────── */

class ScenarioBridgeTest : public ::testing::Test {
protected:
    ScenarioBridge bridge;
};

TEST_F(ScenarioBridgeTest, EmptyTextYieldsDefaults)
{
    QVariantMap m = bridge.parse("");
    EXPECT_TRUE(m["ok"].toBool());
    EXPECT_FALSE(m["scripted"].toBool());
    EXPECT_EQ(m["quantumHi"].toInt(), 3);
    EXPECT_EQ(m["quantumLo"].toInt(), 6);
    EXPECT_EQ(m["seed"].toInt(), 42);
    EXPECT_EQ(m["processCount"].toInt(), 5);
    EXPECT_EQ(m["diskMin"].toInt(), 5);
    EXPECT_EQ(m["tapeMin"].toInt(), 8);
    EXPECT_EQ(m["printerMin"].toInt(), 12);
    EXPECT_EQ(m["pDisk"].toInt(), 34);
    EXPECT_EQ(m["diskMode"].toString(), "concurrent");
}

TEST_F(ScenarioBridgeTest, ParsesRandomScenarioGlobals)
{
    QVariantMap m = bridge.parse(
        "quantum-hi = 4\n"
        "quantum-lo = 8\n"
        "process-count = 7\n"
        "p-io = 30\n"
        "disk-duration = 2-6\n"
        "disk-mode = queue\n");
    EXPECT_TRUE(m["ok"].toBool());
    EXPECT_FALSE(m["scripted"].toBool());
    EXPECT_EQ(m["quantumHi"].toInt(), 4);
    EXPECT_EQ(m["quantumLo"].toInt(), 8);
    EXPECT_EQ(m["processCount"].toInt(), 7);
    EXPECT_EQ(m["pIo"].toInt(), 30);
    EXPECT_EQ(m["diskMin"].toInt(), 2);
    EXPECT_EQ(m["diskMax"].toInt(), 6);
    EXPECT_EQ(m["diskMode"].toString(), "queue");
}

TEST_F(ScenarioBridgeTest, ParsesScriptedProcessesWithIoTimeline)
{
    QVariantMap m = bridge.parse(
        "[process]\n"
        "arrival = 0\n"
        "burst = 10\n"
        "io = 2:disk, 4:tape:6, 7:printer:4-8\n"
        "\n"
        "[process]\n"
        "arrival = 3\n"
        "burst = 5\n");
    ASSERT_TRUE(m["ok"].toBool());
    EXPECT_TRUE(m["scripted"].toBool());
    EXPECT_EQ(m["processCount"].toInt(), 2);

    QVariantList procs = m["processes"].toList();
    ASSERT_EQ(procs.size(), 2);
    QVariantMap p0 = procs[0].toMap();
    EXPECT_EQ(p0["arrival"].toInt(), 0);
    EXPECT_EQ(p0["burst"].toInt(), 10);
    /* io timeline is rendered back in the documented text form */
    EXPECT_EQ(p0["io"].toString(), "2:disk, 4:tape:6, 7:printer:4-8");
    QVariantMap p1 = procs[1].toMap();
    EXPECT_EQ(p1["arrival"].toInt(), 3);
    EXPECT_EQ(p1["io"].toString(), "");
}

TEST_F(ScenarioBridgeTest, InvalidTextReportsError)
{
    QVariantMap m = bridge.parse("no-such-key = 1\n");
    EXPECT_FALSE(m["ok"].toBool());
    EXPECT_FALSE(m["error"].toString().isEmpty());
}

TEST_F(ScenarioBridgeTest, InvalidIoTimelineReportsError)
{
    /* io ticks must be strictly increasing and < burst */
    QVariantMap m = bridge.parse(
        "[process]\narrival = 0\nburst = 4\nio = 6:disk\n");
    EXPECT_FALSE(m["ok"].toBool());
}

TEST_F(ScenarioBridgeTest, FileRoundTrip)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath("t.scn");
    const QString text = "quantum-hi = 9\n[process]\narrival = 1\nburst = 3\n";

    ASSERT_TRUE(bridge.writeFile(path, text));
    EXPECT_EQ(bridge.readFile(path), text);

    QVariantMap m = bridge.summarize(path);
    EXPECT_TRUE(m["ok"].toBool());
    EXPECT_EQ(m["quantumHi"].toInt(), 9);
    EXPECT_TRUE(m["scripted"].toBool());
    EXPECT_EQ(m["path"].toString(), path);
}

TEST_F(ScenarioBridgeTest, SummarizeMissingFileFails)
{
    QVariantMap m = bridge.summarize("/nonexistent/nope.scn");
    EXPECT_FALSE(m["ok"].toBool());
    EXPECT_FALSE(m["error"].toString().isEmpty());
}

TEST_F(ScenarioBridgeTest, ToLocalPathHandlesUrlsAndPlainPaths)
{
    EXPECT_EQ(bridge.toLocalPath("file:///tmp/x.scn"), "/tmp/x.scn");
    EXPECT_EQ(bridge.toLocalPath("/tmp/x.scn"), "/tmp/x.scn");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
