#include "ScenarioBridge.h"

#include <QFile>
#include <QTextStream>
#include <QUrl>

extern "C" {
#include "cutesim/scenario.h"
}

ScenarioBridge::ScenarioBridge(QObject *parent)
    : QObject(parent)
{
}

/* Mirrors default_config() in apps/rr-feedback/args.c so the editor shows the
   effective values for keys a scenario omits. */
static SimConfig bridgeDefaults()
{
    SimConfig cfg        = {};
    cfg.quantum_hi       = 3;
    cfg.quantum_lo       = 6;
    cfg.seed             = 42;
    cfg.process_count    = 5;
    cfg.service_duration = { 5, 15 };
    cfg.disk_duration    = { 5, 5 };
    cfg.tape_duration    = { 8, 8 };
    cfg.printer_duration = { 12, 12 };
    cfg.p_disk           = 34;
    cfg.p_tape           = 33;
    cfg.p_printer        = 33;
    return cfg;
}

static QString ioModeStr(IoMode m)
{
    return m == IO_MODE_QUEUE ? QStringLiteral("queue") : QStringLiteral("concurrent");
}

static QString deviceStr(DeviceType d)
{
    switch (d) {
    case DEVICE_DISK:    return QStringLiteral("disk");
    case DEVICE_TAPE:    return QStringLiteral("tape");
    case DEVICE_PRINTER: return QStringLiteral("printer");
    default:             return QStringLiteral("disk");
    }
}

/* Render a process's I/O timeline back to the "t:dev[:d[-d]], …" text form. */
static QString ioTimelineStr(const ScriptedProcess &p)
{
    QStringList parts;
    for (int i = 0; i < p.io_count; ++i) {
        const ScriptedIO &ev = p.io[i];
        QString s = QString::number(ev.service_tick) + ":" + deviceStr(ev.device);
        if (ev.has_duration) {
            s += ":" + QString::number(ev.duration.min);
            if (ev.duration.max != ev.duration.min)
                s += "-" + QString::number(ev.duration.max);
        }
        parts << s;
    }
    return parts.join(QStringLiteral(", "));
}

QVariantMap ScenarioBridge::parse(const QString &text) const
{
    QVariantMap out;

    Scenario sc = {};
    sc.config   = bridgeDefaults();

    QByteArray utf8 = text.toUtf8();
    if (scenario_parse_string(utf8.constData(), &sc) != 0) {
        scenario_free(&sc);
        out["ok"]    = false;
        out["error"] = QStringLiteral("invalid scenario (syntax or validation error)");
        return out;
    }

    const SimConfig &c = sc.config;
    out["ok"]           = true;
    out["error"]        = QString();
    out["scripted"]     = sc.process_count > 0;
    out["processCount"] = sc.process_count > 0 ? sc.process_count : c.process_count;
    out["quantumHi"]    = c.quantum_hi;
    out["quantumLo"]    = c.quantum_lo;
    out["seed"]         = (int)c.seed;
    out["pIo"]          = c.p_io;
    out["pDisk"]        = c.p_disk;
    out["pTape"]        = c.p_tape;
    out["pPrinter"]     = c.p_printer;
    out["serviceMin"]   = c.service_duration.min;
    out["serviceMax"]   = c.service_duration.max;
    out["diskMin"]      = c.disk_duration.min;
    out["diskMax"]      = c.disk_duration.max;
    out["tapeMin"]      = c.tape_duration.min;
    out["tapeMax"]      = c.tape_duration.max;
    out["printerMin"]   = c.printer_duration.min;
    out["printerMax"]   = c.printer_duration.max;
    out["diskMode"]     = ioModeStr(c.io_mode_disk);
    out["tapeMode"]     = ioModeStr(c.io_mode_tape);
    out["printerMode"]  = ioModeStr(c.io_mode_printer);

    QVariantList procs;
    for (int i = 0; i < sc.process_count; ++i) {
        const ScriptedProcess &p = sc.processes[i];
        QVariantMap m;
        m["arrival"] = p.arrival_tick;
        m["burst"]   = p.burst;
        m["io"]      = ioTimelineStr(p);
        procs << m;
    }
    out["processes"] = procs;

    scenario_free(&sc);
    return out;
}

QVariantMap ScenarioBridge::summarize(const QString &pathOrUrl) const
{
    const QString path = toLocalPath(pathOrUrl);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QVariantMap out;
        out["ok"]    = false;
        out["error"] = QStringLiteral("cannot read file: %1").arg(path);
        return out;
    }
    QVariantMap out = parse(QString::fromUtf8(f.readAll()));
    out["path"]     = path;
    return out;
}

QString ScenarioBridge::readFile(const QString &pathOrUrl) const
{
    QFile f(toLocalPath(pathOrUrl));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

bool ScenarioBridge::writeFile(const QString &pathOrUrl, const QString &text) const
{
    QFile f(toLocalPath(pathOrUrl));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    ts << text;
    return true;
}

QString ScenarioBridge::toLocalPath(const QString &pathOrUrl) const
{
    if (pathOrUrl.startsWith(QLatin1String("file:")))
        return QUrl(pathOrUrl).toLocalFile();
    return pathOrUrl;
}
