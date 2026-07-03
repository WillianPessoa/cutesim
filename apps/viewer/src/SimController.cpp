#include "SimController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QTimer>
#include <algorithm>

SimController::SimController(QObject *parent)
    : QObject(parent)
    , m_client(new SimClient(this))
{
    connect(m_client, &SimClient::snapshotReceived, this, &SimController::onSnapshot);
    connect(m_client, &SimClient::connected,        this, &SimController::onConnected);
    connect(m_client, &SimClient::disconnected,     this, &SimController::onDisconnected);
    connect(m_client, &SimClient::errorOccurred,    this, &SimController::onClientError);
}

SimController::~SimController()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(1000);
    }
}

/* ── Public invokables ──────────────────────────────────────────────── */

void SimController::connectToServer(const QString &host, quint16 port)
{
    m_client->connectToServer(host, port);
}

void SimController::step()  { m_client->step();  }
void SimController::reset()
{
    clearState();
    emit stateUpdated();
    emit connectionChanged();
    m_client->reset();
}

void SimController::reconfigure()
{
    clearState();
    emit stateUpdated();
    emit connectionChanged();

    m_launching = true;
    emit launchStateChanged();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(800);
    }
    delete m_process;
    m_process = nullptr;

    m_launching   = false;
    m_needsLaunch = true;
    emit launchStateChanged();
}

void SimController::launch(const QVariantMap &params)
{
    m_lastParams = params;
    emit launchStateChanged();
    launchBinary(params);
}

/* ── Private: launch ────────────────────────────────────────────────── */

QStringList SimController::buildArgs(const QVariantMap &params)
{
    const int port = params.value("port", 9000).toInt();
    QStringList args;

    const QString scn = params.value("scenarioFile").toString();
    if (!scn.isEmpty()) {
        /* Scenario mode: the file supplies the whole config; file values take
           precedence over flags in the binary anyway. */
        args << scn << QString("--serve=%1").arg(port);
        return args;
    }

    args << QString("--process-count=%1").arg(params.value("processes",  5).toInt())
         << QString("--quantum-hi=%1").arg(params.value("quantumHi",  3).toInt())
         << QString("--quantum-lo=%1").arg(params.value("quantumLo",  6).toInt())
         << QString("--p-io=%1").arg(params.value("pIo",  0).toInt())
         << QString("--service-duration=%1-%2")
                .arg(params.value("serviceMin",  5).toInt())
                .arg(params.value("serviceMax", 15).toInt())
         << QString("--seed=%1").arg(params.value("seed", 42).toInt());

    const QString arrival = params.value("arrivalMode", "batch").toString();
    if (arrival == "bernoulli" || arrival == "geometric") {
        args << "--arrival-mode=bernoulli"
             << QString("--arrival-rate=%1").arg(params.value("arrivalRate", 20).toInt());
    } else if (arrival == "poisson") {
        args << "--arrival-mode=poisson"
             << QString("--arrival-lambda=%1").arg(params.value("arrivalLambda", 0.5).toDouble());
    } else if (arrival == "uniform") {
        args << "--arrival-mode=uniform"
             << QString("--arrival-interval=%1").arg(params.value("arrivalInterval", 3).toInt());
    }

    /* Device split: pass disk + tape only — the binary assigns the remainder
       to the printer, so the three always sum to 100. */
    if (params.contains("pDisk") && params.contains("pTape")) {
        args << QString("--p-disk=%1").arg(params.value("pDisk").toInt())
             << QString("--p-tape=%1").arg(params.value("pTape").toInt());
    }

    const struct { const char *minKey, *maxKey, *modeKey, *durFlag, *modeFlag; } devs[] = {
        { "diskMin",    "diskMax",    "diskMode",    "--disk-duration",    "--io-mode-disk"    },
        { "tapeMin",    "tapeMax",    "tapeMode",    "--tape-duration",    "--io-mode-tape"    },
        { "printerMin", "printerMax", "printerMode", "--printer-duration", "--io-mode-printer" },
    };
    for (const auto &d : devs) {
        if (params.contains(d.minKey) && params.contains(d.maxKey)) {
            args << QString("%1=%2-%3").arg(d.durFlag)
                        .arg(params.value(d.minKey).toInt())
                        .arg(params.value(d.maxKey).toInt());
        }
        if (params.contains(d.modeKey))
            args << QString("%1=%2").arg(d.modeFlag, params.value(d.modeKey).toString());
    }

    args << QString("--serve=%1").arg(port);
    return args;
}

void SimController::launchBinary(const QVariantMap &params)
{
    QString bin = findBinary();
    if (bin.isEmpty()) {
        emit errorOccurred("rr-feedback binary not found. Set CUTESIM_BIN or build first.");
        return;
    }

    if (m_process) {
        m_process->terminate();
        m_process->waitForFinished(500);
        delete m_process;
        m_process = nullptr;
    }
    clearState();

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        qDebug().noquote() << "[rr-feedback]" << m_process->readAllStandardError().trimmed();
    });

    QVariantMap p = params;
    p["port"] = choosePort(params);

    const int   port = p.value("port").toInt();
    QStringList args = buildArgs(p);

    m_launching = true;
    emit launchStateChanged();

    m_process->start(bin, args);
    if (!m_process->waitForStarted(2000)) {
        emit errorOccurred("Failed to start rr-feedback: " + m_process->errorString());
        m_launching = false;
        emit launchStateChanged();
        return;
    }

    QTimer::singleShot(300, this, [this, port]() {
        m_needsLaunch = false;
        m_launching   = false;
        emit launchStateChanged();
        connectToServer("127.0.0.1", static_cast<quint16>(port));
    });
}

int SimController::choosePort(const QVariantMap &params)
{
    if (params.contains("port"))
        return params.value("port").toInt();
    return 20000 + int(QRandomGenerator::global()->bounded(40000));
}

QString SimController::findBinary()
{
    QByteArray env = qgetenv("CUTESIM_BIN");
    if (!env.isEmpty()) {
        QString s = QString::fromLocal8Bit(env);
        if (QFileInfo::exists(s)) return s;
    }

    QString base = QCoreApplication::applicationDirPath();
    if (QFileInfo::exists(base + "/rr-feedback")) return base + "/rr-feedback";

    QDir dir(base);
    for (int i = 0; i < 8; ++i) {
        QString c = dir.filePath("build/apps/rr-feedback/rr-feedback");
        if (QFileInfo::exists(c)) return c;
        if (!dir.cdUp()) break;
    }
    return {};
}

/* ── Private slots ──────────────────────────────────────────────────── */

void SimController::onConnected()
{
    m_connected   = true;
    m_needsLaunch = false;
    emit connectionChanged();
    emit launchStateChanged();
    emit infoOccurred("● connected to rr-feedback");
}

void SimController::onDisconnected()
{
    m_connected = false;
    emit connectionChanged();
}

void SimController::onClientError(const QString &msg)
{
    if (m_launching) return;

    if (!m_connected && !m_needsLaunch) {
        m_needsLaunch = true;
        emit launchStateChanged();
    }
    emit errorOccurred(msg);
}

void SimController::onSnapshot(const QJsonObject &snap, const QString &raw)
{
    /* A snapshot for a tick we already processed (e.g. a "step" answered after
       the sim finished, or a "status" query) must not append to the histories,
       or the Gantt/sparklines grow phantom ticks. State overwrites are
       idempotent and stay unconditional.

       Tick 0 is the pre-simulation state (the reply to "reset"): nothing ran
       yet, so recording it would paint a fake idle tick at the start of the
       Gantt. The first simulated tick is 1. */
    const int  tick    = snap["tick"].toInt();
    const bool newTick = !m_hasSnapshot || tick != m_tick;
    const bool record  = newTick && tick > 0;
    if (newTick)
        m_prevEvents = m_events; /* events of the previous tick */

    m_hasSnapshot = true;
    m_rawSnapshot = raw;
    m_tick        = tick;
    m_done        = snap["done"].toBool();

    /* ── CPU ─────────────────────────────────────────────────────────── */
    QJsonValue cpuVal = snap["cpu"];
    m_cpu.clear();
    if (!cpuVal.isNull() && !cpuVal.isUndefined()) {
        QJsonObject c    = cpuVal.toObject();
        m_cpu["pid"]            = c["pid"].toInt();
        m_cpu["remaining"]      = c["remaining"].toInt();
        m_cpu["quantum_used"]   = c["quantum_used"].toInt();
        m_cpu["quantum_max"]    = c["quantum_max"].toInt();
        m_cpu["queue"]          = c["queue"].toString();
        m_cpu["burst_remaining"] = c["burst_remaining"].toInt(-1);
        if (c.contains("ghost"))  m_cpu["ghost"]  = c["ghost"].toString();
        if (c.contains("device")) m_cpu["device"] = c["device"].toString();
        /* An io_start ghost did not consume a CPU tick (Model A) — the CPU
           card shows where the process went, but the histories stay truthful
           and record the tick as idle. */
        const bool ranThisTick = c["ghost"].toString() != QLatin1String("io_start");
        if (record) {
            appendCapped(m_ganttHistory, ranThisTick ? c["pid"].toInt() : -1);
            appendCapped(m_cpuHistory,   ranThisTick ? c["pid"].toInt() : 0);
        }
    } else if (record) {
        appendCapped(m_ganttHistory, -1);
        appendCapped(m_cpuHistory,   0);
    }

    /* ── Queues ──────────────────────────────────────────────────────── */
    QJsonObject queues = snap["queues"].toObject();
    m_highQueue    = parseProcessArray(queues["high"].toArray());
    m_lowQueue     = parseProcessArray(queues["low"].toArray());
    m_diskQueue    = parseProcessArray(queues["disk"].toArray());
    m_tapeQueue    = parseProcessArray(queues["tape"].toArray());
    m_printerQueue = parseProcessArray(queues["printer"].toArray());

    /* ── Finished ────────────────────────────────────────────────────── */
    m_finished = parseProcessArray(snap["finished"].toArray());

    /* ── Stats ───────────────────────────────────────────────────────── */
    QJsonObject st     = snap["stats"].toObject();
    double cpuUtil     = st["cpu_utilization"].toDouble();
    double avgTa       = st["avg_turnaround"].toDouble();
    double throughput  = st["throughput"].toDouble();

    m_stats["cpu_utilization"] = cpuUtil;
    m_stats["throughput"]      = throughput;
    m_stats["avg_turnaround"]  = avgTa;
    m_stats["avg_waiting"]     = st["avg_waiting"].toDouble();
    m_stats["avg_response"]    = st["avg_response"].toDouble();

    if (record) {
        appendCapped(m_utilHistory,       cpuUtil);
        appendCapped(m_turnaroundHistory, avgTa);
        appendCapped(m_throughputHistory, throughput);
    }

    /* ── Events ──────────────────────────────────────────────────────── */
    m_events.clear();
    for (const QJsonValue &ev : snap["events"].toArray()) {
        QJsonObject e = ev.toObject();
        QVariantMap m;
        m["type"] = e["type"].toString();
        m["pid"]  = e["pid"].toInt();
        if (e.contains("queue"))        m["queue"]        = e["queue"].toString();
        if (e.contains("device"))       m["device"]       = e["device"].toString();
        if (e.contains("io_remaining")) m["io_remaining"] = e["io_remaining"].toInt();
        if (e.contains("quantum_used"))  m["quantum_used"]  = e["quantum_used"].toInt();
        if (e.contains("quantum_max"))  m["quantum_max"]  = e["quantum_max"].toInt();
        m_events.append(m);
    }
    if (record) {
        appendCapped(m_eventsHistory, m_events);
        appendCapped(m_rawHistory, raw);
    }

    /* ── allProcesses + totalProcessCount ───────────────────────────── */
    {
        auto recordSeen = [&](int pid) {
            if (pid > 0 && !m_firstSeenTick.contains(pid))
                m_firstSeenTick[pid] = m_tick;
        };
        if (!m_cpu.isEmpty()) recordSeen(m_cpu["pid"].toInt());
        for (const QVariant &v : std::as_const(m_highQueue))    recordSeen(v.toMap()["pid"].toInt());
        for (const QVariant &v : std::as_const(m_lowQueue))     recordSeen(v.toMap()["pid"].toInt());
        for (const QVariant &v : std::as_const(m_diskQueue))    recordSeen(v.toMap()["pid"].toInt());
        for (const QVariant &v : std::as_const(m_tapeQueue))    recordSeen(v.toMap()["pid"].toInt());
        for (const QVariant &v : std::as_const(m_printerQueue)) recordSeen(v.toMap()["pid"].toInt());
        for (const QVariant &v : std::as_const(m_finished))     recordSeen(v.toMap()["pid"].toInt());

        QHash<int, QVariantMap> active;
        auto addActive = [&](const QVariantList &lst, const QString &status) {
            for (const QVariant &v : lst) {
                QVariantMap m = v.toMap();
                int pid = m["pid"].toInt();
                if (pid > 0) {
                    m["status"]          = status;
                    m["first_seen_tick"] = m_firstSeenTick.value(pid, m_tick);
                    active[pid]          = m;
                }
            }
        };
        if (!m_cpu.isEmpty()) {
            QVariantMap m = m_cpu;
            m["status"]          = "cpu_" + m_cpu["queue"].toString();
            m["first_seen_tick"] = m_firstSeenTick.value(m_cpu["pid"].toInt(), m_tick);
            active[m_cpu["pid"].toInt()] = m;
        }
        addActive(m_highQueue,    "queue_high");
        addActive(m_lowQueue,     "queue_low");
        addActive(m_diskQueue,    "io_disk");
        addActive(m_tapeQueue,    "io_tape");
        addActive(m_printerQueue, "io_printer");

        QVariantList all;
        QSet<int> donePids;
        for (const QVariant &v : std::as_const(m_finished)) {
            QVariantMap m = v.toMap();
            m["status"]          = "done";
            m["first_seen_tick"] = m_firstSeenTick.value(m["pid"].toInt(), 0);
            all.append(m);
            donePids.insert(m["pid"].toInt());
        }
        for (auto it = active.cbegin(); it != active.cend(); ++it) {
            if (!donePids.contains(it.key()))
                all.append(it.value());
        }
        std::sort(all.begin(), all.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap()["pid"].toInt() < b.toMap()["pid"].toInt();
        });
        m_allProcesses = all;
        m_totalProcessCount = qMax(m_totalProcessCount, (int)m_firstSeenTick.size());
    }

    emit stateUpdated();
}

/* ── Helpers ────────────────────────────────────────────────────────── */

void SimController::clearState()
{
    m_tick = 0; m_done = false;
    m_cpu.clear();
    m_highQueue.clear(); m_lowQueue.clear();
    m_diskQueue.clear(); m_tapeQueue.clear(); m_printerQueue.clear();
    m_finished.clear(); m_stats.clear(); m_events.clear();
    m_prevEvents.clear();
    m_rawSnapshot.clear();
    m_ganttHistory.clear(); m_allProcesses.clear();
    m_cpuHistory.clear(); m_eventsHistory.clear(); m_rawHistory.clear();
    m_utilHistory.clear();
    m_turnaroundHistory.clear(); m_throughputHistory.clear();
    m_firstSeenTick.clear();
    m_totalProcessCount = 0;
    m_hasSnapshot = false;
    /* m_connected is deliberately untouched: connection state belongs to the
       socket (onConnected/onDisconnected), and reset() clears sim state while
       the connection stays open. */
}

QVariantList SimController::parseProcessArray(const QJsonArray &arr)
{
    QVariantList list;
    list.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        QVariantMap m;
        if (o.contains("pid"))          m["pid"]          = o["pid"].toInt();
        if (o.contains("remaining"))    m["remaining"]    = o["remaining"].toInt();
        if (o.contains("io_remaining")) m["io_remaining"] = o["io_remaining"].toInt();
        /* finished fields */
        if (o.contains("arrival_tick")) m["arrival_tick"] = o["arrival_tick"].toInt();
        if (o.contains("finish_tick"))  m["finish_tick"]  = o["finish_tick"].toInt();
        if (o.contains("service_time")) m["service_time"] = o["service_time"].toInt();
        if (o.contains("cpu_time_used"))m["cpu_time_used"]= o["cpu_time_used"].toInt();
        if (o.contains("io_disk"))      m["io_disk"]      = o["io_disk"].toInt();
        if (o.contains("io_tape"))      m["io_tape"]      = o["io_tape"].toInt();
        if (o.contains("io_printer"))   m["io_printer"]   = o["io_printer"].toInt();
        if (o.contains("io_total"))     m["io_total"]     = o["io_total"].toInt();
        if (o.contains("io_count"))     m["io_count"]     = o["io_count"].toInt();
        if (o.contains("wait_time"))    m["wait_time"]    = o["wait_time"].toInt();
        if (o.contains("turnaround"))   m["turnaround"]   = o["turnaround"].toInt();
        if (o.contains("response_time"))m["response_time"]= o["response_time"].toInt();
        list.append(m);
    }
    return list;
}

void SimController::appendCapped(QVariantList &list, const QVariant &v)
{
    list.append(v);
    if (list.size() > kMaxHistory) list.removeFirst();
}
