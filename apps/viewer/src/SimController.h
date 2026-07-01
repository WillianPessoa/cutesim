#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QVariantMap>

#include "SimClient.h"

/* SimController — QML-facing bridge to rr-feedback --serve.
   Parses JSON Lines snapshots from SimClient and exposes derived state as
   Q_PROPERTYs. All simulation state is updated in a single stateUpdated()
   signal per tick — QML bindings update atomically.

   Key simplification over the old SchedulerController: the CuteSim snapshot
   already includes an events[] array encoding all transitions (arrived,
   scheduled, preempted, io_start, io_return, completed). No m_prev* diffs. */
class SimController : public QObject
{
    Q_OBJECT

    /* ── Simulation state ─────────────────────────────────────────────── */
    Q_PROPERTY(int          tick          READ tick          NOTIFY stateUpdated)
    Q_PROPERTY(QVariantMap  cpu           READ cpu           NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList highQueue     READ highQueue     NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList lowQueue      READ lowQueue      NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList diskQueue     READ diskQueue     NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList tapeQueue     READ tapeQueue     NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList printerQueue  READ printerQueue  NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList finished      READ finished      NOTIFY stateUpdated)
    Q_PROPERTY(QVariantMap  stats         READ stats         NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList events        READ events        NOTIFY stateUpdated)
    Q_PROPERTY(QString      rawSnapshot   READ rawSnapshot   NOTIFY stateUpdated)
    Q_PROPERTY(bool         done          READ isDone        NOTIFY stateUpdated)

    /* ── Derived histories ────────────────────────────────────────────── */
    Q_PROPERTY(QVariantList ganttHistory      READ ganttHistory      NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList allProcesses      READ allProcesses      NOTIFY stateUpdated)
    Q_PROPERTY(int          totalProcessCount READ totalProcessCount NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList cpuHistory        READ cpuHistory        NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList utilHistory       READ utilHistory       NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList turnaroundHistory READ turnaroundHistory NOTIFY stateUpdated)
    Q_PROPERTY(QVariantList throughputHistory READ throughputHistory NOTIFY stateUpdated)

    /* ── Connection state ─────────────────────────────────────────────── */
    Q_PROPERTY(bool connected  READ isConnected  NOTIFY connectionChanged)
    Q_PROPERTY(bool simDone    READ isSimDone    NOTIFY connectionChanged)

    /* ── Launch state ─────────────────────────────────────────────────── */
    Q_PROPERTY(bool        needsLaunch READ needsLaunch NOTIFY launchStateChanged)
    Q_PROPERTY(bool        launching   READ isLaunching NOTIFY launchStateChanged)
    Q_PROPERTY(QVariantMap lastParams  READ lastParams  NOTIFY launchStateChanged)

public:
    explicit SimController(QObject *parent = nullptr);
    ~SimController();

    /* ── Simulation state getters ─────────────────────────────────────── */
    int          tick()          const { return m_tick; }
    QVariantMap  cpu()           const { return m_cpu; }
    QVariantList highQueue()     const { return m_highQueue; }
    QVariantList lowQueue()      const { return m_lowQueue; }
    QVariantList diskQueue()     const { return m_diskQueue; }
    QVariantList tapeQueue()     const { return m_tapeQueue; }
    QVariantList printerQueue()  const { return m_printerQueue; }
    QVariantList finished()      const { return m_finished; }
    QVariantMap  stats()         const { return m_stats; }
    QVariantList events()        const { return m_events; }
    QString      rawSnapshot()   const { return m_rawSnapshot; }
    bool         isDone()        const { return m_done; }

    /* ── History getters ──────────────────────────────────────────────── */
    QVariantList ganttHistory()      const { return m_ganttHistory; }
    QVariantList allProcesses()      const { return m_allProcesses; }
    int          totalProcessCount() const { return m_totalProcessCount; }
    QVariantList cpuHistory()        const { return m_cpuHistory; }
    QVariantList utilHistory()       const { return m_utilHistory; }
    QVariantList turnaroundHistory() const { return m_turnaroundHistory; }
    QVariantList throughputHistory() const { return m_throughputHistory; }

    /* ── Connection / launch getters ──────────────────────────────────── */
    bool        isConnected() const { return m_connected; }
    bool        isSimDone()   const { return m_simDone; }
    bool        needsLaunch() const { return m_needsLaunch; }
    bool        isLaunching() const { return m_launching; }
    QVariantMap lastParams()  const { return m_lastParams; }

public slots:
    Q_INVOKABLE void connectToServer(const QString &host = "127.0.0.1", quint16 port = 9000);
    Q_INVOKABLE void step();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void reconfigure();
    Q_INVOKABLE void launch(const QVariantMap &params);

signals:
    void stateUpdated();
    void connectionChanged();
    void launchStateChanged();
    void errorOccurred(const QString &message);
    void infoOccurred(const QString &message);

private slots:
    void onSnapshot(const QJsonObject &snap, const QString &raw);
    void onConnected();
    void onDisconnected();
    void onClientError(const QString &msg);

private:
    void clearState();
    void launchBinary(const QVariantMap &params);
    static QString findBinary();
    static QVariantList parseProcessArray(const QJsonArray &arr);

    static constexpr int kMaxHistory = 512;
    static void appendCapped(QVariantList &list, const QVariant &v);

    SimClient *m_client;
    QProcess  *m_process = nullptr;

    /* ── Simulation state ─────────────────────────────────────────────── */
    int          m_tick          = 0;
    QVariantMap  m_cpu;
    QVariantList m_highQueue;
    QVariantList m_lowQueue;
    QVariantList m_diskQueue;
    QVariantList m_tapeQueue;
    QVariantList m_printerQueue;
    QVariantList m_finished;
    QVariantMap  m_stats;
    QVariantList m_events;
    QString      m_rawSnapshot;
    bool         m_done          = false;

    /* ── Histories ────────────────────────────────────────────────────── */
    QVariantList m_ganttHistory;
    QVariantList m_allProcesses;
    int          m_totalProcessCount = 0;
    QVariantList m_cpuHistory;
    QVariantList m_utilHistory;
    QVariantList m_turnaroundHistory;
    QVariantList m_throughputHistory;
    QHash<int, int> m_firstSeenTick;

    /* ── Connection / launch ──────────────────────────────────────────── */
    bool        m_connected   = false;
    bool        m_simDone     = false;
    bool        m_needsLaunch = true;
    bool        m_launching   = false;
    QVariantMap m_lastParams;
};
