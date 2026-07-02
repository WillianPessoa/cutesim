#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include "SimController.h"

/* Helpers ──────────────────────────────────────────────────────────────── */

/* Invoke the private slot SimController::onSnapshot directly. */
static void feedSnapshot(SimController &c, const QJsonObject &snap)
{
    QByteArray raw = QJsonDocument(snap).toJson(QJsonDocument::Compact);
    QMetaObject::invokeMethod(&c, "onSnapshot", Qt::DirectConnection,
                              Q_ARG(QJsonObject, snap),
                              Q_ARG(QString, QString::fromUtf8(raw)));
}

/* Build a minimal valid snapshot object. */
static QJsonObject baseSnap(int tick = 1)
{
    QJsonObject s;
    s["tick"]  = tick;
    s["done"]  = false;
    s["cpu"]   = QJsonValue::Null;
    s["queues"] = QJsonObject{
        {"high", QJsonArray{}}, {"low", QJsonArray{}},
        {"disk", QJsonArray{}}, {"tape", QJsonArray{}}, {"printer", QJsonArray{}}
    };
    s["finished"] = QJsonArray{};
    s["stats"] = QJsonObject{
        {"cpu_utilization", 0.0}, {"avg_turnaround", 0.0},
        {"avg_waiting", 0.0},     {"avg_response", 0.0},
        {"throughput", 0.0}
    };
    s["events"] = QJsonArray{};
    return s;
}

/* Tests ────────────────────────────────────────────────────────────────── */

class SimControllerTest : public ::testing::Test {
protected:
    SimController ctrl;
};

TEST_F(SimControllerTest, InitialState)
{
    EXPECT_EQ(ctrl.tick(), 0);
    EXPECT_FALSE(ctrl.isDone());
    EXPECT_TRUE(ctrl.cpu().isEmpty());
    EXPECT_TRUE(ctrl.highQueue().isEmpty());
    EXPECT_TRUE(ctrl.lowQueue().isEmpty());
    EXPECT_TRUE(ctrl.diskQueue().isEmpty());
    EXPECT_TRUE(ctrl.tapeQueue().isEmpty());
    EXPECT_TRUE(ctrl.printerQueue().isEmpty());
    EXPECT_TRUE(ctrl.finished().isEmpty());
    EXPECT_TRUE(ctrl.events().isEmpty());
    EXPECT_TRUE(ctrl.rawSnapshot().isEmpty());
    EXPECT_EQ(ctrl.totalProcessCount(), 0);
    EXPECT_FALSE(ctrl.isConnected());
    EXPECT_TRUE(ctrl.needsLaunch());
    EXPECT_FALSE(ctrl.isLaunching());
}

TEST_F(SimControllerTest, SnapshotUpdatesTick)
{
    feedSnapshot(ctrl, baseSnap(42));
    EXPECT_EQ(ctrl.tick(), 42);
}

TEST_F(SimControllerTest, SnapshotSetsDoneFlag)
{
    QJsonObject s = baseSnap(5);
    s["done"] = true;
    feedSnapshot(ctrl, s);
    EXPECT_TRUE(ctrl.isDone());
}

TEST_F(SimControllerTest, SnapshotActiveCpu)
{
    QJsonObject cpu;
    cpu["pid"]          = 3;
    cpu["remaining"]    = 7;
    cpu["quantum_used"] = 2;
    cpu["quantum_max"]  = 3;
    cpu["queue"]        = "high";

    QJsonObject s = baseSnap(1);
    s["cpu"] = cpu;
    feedSnapshot(ctrl, s);

    ASSERT_FALSE(ctrl.cpu().isEmpty());
    EXPECT_EQ(ctrl.cpu()["pid"].toInt(), 3);
    EXPECT_EQ(ctrl.cpu()["remaining"].toInt(), 7);
    EXPECT_EQ(ctrl.cpu()["quantum_used"].toInt(), 2);
    EXPECT_EQ(ctrl.cpu()["quantum_max"].toInt(), 3);
    EXPECT_EQ(ctrl.cpu()["queue"].toString(), "high");
}

TEST_F(SimControllerTest, SnapshotNullCpuClearsField)
{
    // First put something on CPU
    QJsonObject cpu;
    cpu["pid"] = 1; cpu["remaining"] = 1;
    cpu["quantum_used"] = 1; cpu["quantum_max"] = 3; cpu["queue"] = "low";
    QJsonObject s = baseSnap(1);
    s["cpu"] = cpu;
    feedSnapshot(ctrl, s);
    ASSERT_FALSE(ctrl.cpu().isEmpty());

    // Then send null CPU
    QJsonObject s2 = baseSnap(2);
    s2["cpu"] = QJsonValue::Null;
    feedSnapshot(ctrl, s2);
    EXPECT_TRUE(ctrl.cpu().isEmpty());
}

TEST_F(SimControllerTest, SnapshotQueues)
{
    QJsonObject p1; p1["pid"] = 1; p1["remaining"] = 4;
    QJsonObject p2; p2["pid"] = 2; p2["remaining"] = 2;
    QJsonObject p3; p3["pid"] = 3; p3["io_remaining"] = 3;

    QJsonObject s = baseSnap(1);
    s["queues"] = QJsonObject{
        {"high",    QJsonArray{p1}},
        {"low",     QJsonArray{p2}},
        {"disk",    QJsonArray{p3}},
        {"tape",    QJsonArray{}},
        {"printer", QJsonArray{}}
    };
    feedSnapshot(ctrl, s);

    ASSERT_EQ(ctrl.highQueue().size(), 1);
    EXPECT_EQ(ctrl.highQueue()[0].toMap()["pid"].toInt(), 1);

    ASSERT_EQ(ctrl.lowQueue().size(), 1);
    EXPECT_EQ(ctrl.lowQueue()[0].toMap()["pid"].toInt(), 2);

    ASSERT_EQ(ctrl.diskQueue().size(), 1);
    EXPECT_EQ(ctrl.diskQueue()[0].toMap()["io_remaining"].toInt(), 3);

    EXPECT_TRUE(ctrl.tapeQueue().isEmpty());
    EXPECT_TRUE(ctrl.printerQueue().isEmpty());
}

TEST_F(SimControllerTest, SnapshotEventsPreempted)
{
    QJsonObject ev;
    ev["type"]         = "preempted";
    ev["pid"]          = 2;
    ev["quantum_used"] = 3;
    ev["quantum_max"]  = 3;

    QJsonObject s = baseSnap(1);
    s["events"] = QJsonArray{ev};
    feedSnapshot(ctrl, s);

    ASSERT_EQ(ctrl.events().size(), 1);
    QVariantMap e = ctrl.events()[0].toMap();
    EXPECT_EQ(e["type"].toString(), "preempted");
    EXPECT_EQ(e["pid"].toInt(), 2);
    EXPECT_EQ(e["quantum_used"].toInt(), 3);
    EXPECT_EQ(e["quantum_max"].toInt(), 3);
}

TEST_F(SimControllerTest, SnapshotEventsIoStart)
{
    QJsonObject ev;
    ev["type"]   = "io_start";
    ev["pid"]    = 1;
    ev["device"] = "disk";

    QJsonObject s = baseSnap(1);
    s["events"] = QJsonArray{ev};
    feedSnapshot(ctrl, s);

    ASSERT_EQ(ctrl.events().size(), 1);
    QVariantMap e = ctrl.events()[0].toMap();
    EXPECT_EQ(e["type"].toString(), "io_start");
    EXPECT_EQ(e["device"].toString(), "disk");
}

TEST_F(SimControllerTest, SnapshotStats)
{
    QJsonObject s = baseSnap(10);
    s["stats"] = QJsonObject{
        {"cpu_utilization", 75.5},
        {"avg_turnaround",  12.3},
        {"avg_waiting",      4.1},
        {"avg_response",     2.0},
        {"throughput",       0.08}
    };
    feedSnapshot(ctrl, s);

    QVariantMap st = ctrl.stats();
    EXPECT_DOUBLE_EQ(st["cpu_utilization"].toDouble(), 75.5);
    EXPECT_DOUBLE_EQ(st["avg_turnaround"].toDouble(),  12.3);
    EXPECT_DOUBLE_EQ(st["avg_waiting"].toDouble(),      4.1);
    EXPECT_DOUBLE_EQ(st["throughput"].toDouble(),       0.08);
}

TEST_F(SimControllerTest, SnapshotFinishedFields)
{
    QJsonObject p;
    p["pid"]          = 5;
    p["arrival_tick"] = 0;
    p["finish_tick"]  = 20;
    p["service_time"] = 15;
    p["cpu_time_used"]= 12;
    p["io_disk"]      = 2;
    p["io_tape"]      = 0;
    p["io_printer"]   = 1;
    p["io_total"]     = 3;
    p["io_count"]     = 2;
    p["wait_time"]    = 5;
    p["turnaround"]   = 20;
    p["response_time"]= 1;

    QJsonObject s = baseSnap(20);
    s["done"]     = true;
    s["finished"] = QJsonArray{p};
    feedSnapshot(ctrl, s);

    ASSERT_EQ(ctrl.finished().size(), 1);
    QVariantMap f = ctrl.finished()[0].toMap();
    EXPECT_EQ(f["pid"].toInt(),          5);
    EXPECT_EQ(f["finish_tick"].toInt(), 20);
    EXPECT_EQ(f["io_disk"].toInt(),      2);
    EXPECT_EQ(f["io_tape"].toInt(),      0);
    EXPECT_EQ(f["io_printer"].toInt(),   1);
    EXPECT_EQ(f["io_total"].toInt(),     3);
    EXPECT_EQ(f["turnaround"].toInt(),  20);
}

TEST_F(SimControllerTest, StateUpdatedSignalEmitted)
{
    QSignalSpy spy(&ctrl, &SimController::stateUpdated);
    feedSnapshot(ctrl, baseSnap(1));
    EXPECT_EQ(spy.count(), 1);
    feedSnapshot(ctrl, baseSnap(2));
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(SimControllerTest, GanttHistoryAccumulates)
{
    for (int i = 1; i <= 5; ++i)
        feedSnapshot(ctrl, baseSnap(i));
    // 5 ticks, each with null CPU → -1 appended
    EXPECT_EQ(ctrl.ganttHistory().size(), 5);
}

TEST_F(SimControllerTest, GanttHistoryCappedAt512)
{
    QJsonObject cpu;
    cpu["pid"] = 1; cpu["remaining"] = 999;
    cpu["quantum_used"] = 1; cpu["quantum_max"] = 6; cpu["queue"] = "low";

    for (int i = 1; i <= 600; ++i) {
        QJsonObject s = baseSnap(i);
        s["cpu"] = cpu;
        feedSnapshot(ctrl, s);
    }
    // kMaxHistory = 512
    EXPECT_EQ(ctrl.ganttHistory().size(), 512);
}

TEST_F(SimControllerTest, AllProcessesMergesLiveAndFinished)
{
    // Send tick with P1 on CPU and P2 done
    QJsonObject cpu;
    cpu["pid"] = 1; cpu["remaining"] = 2;
    cpu["quantum_used"] = 1; cpu["quantum_max"] = 3; cpu["queue"] = "high";

    QJsonObject done;
    done["pid"] = 2; done["arrival_tick"] = 0; done["finish_tick"] = 5;
    done["service_time"] = 5; done["cpu_time_used"] = 5;
    done["io_disk"] = 0; done["io_tape"] = 0; done["io_printer"] = 0;
    done["io_total"] = 0; done["io_count"] = 0;
    done["wait_time"] = 0; done["turnaround"] = 5; done["response_time"] = 0;

    QJsonObject s = baseSnap(6);
    s["cpu"]      = cpu;
    s["finished"] = QJsonArray{done};
    feedSnapshot(ctrl, s);

    EXPECT_GE(ctrl.allProcesses().size(), 2);
    EXPECT_EQ(ctrl.totalProcessCount(), 2);
}

TEST_F(SimControllerTest, RawSnapshotStored)
{
    feedSnapshot(ctrl, baseSnap(1));
    EXPECT_FALSE(ctrl.rawSnapshot().isEmpty());
    EXPECT_TRUE(ctrl.rawSnapshot().contains("tick"));
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
