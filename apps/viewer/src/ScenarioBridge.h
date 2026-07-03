#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/* ScenarioBridge — QML-facing wrapper around the C scenario parser.
   Single source of truth for .scn handling: the editor and the launch overlay
   validate with exactly the same code the rr-feedback binary runs.

   parse() fills defaults matching the binary's default_config() before
   applying the text, so the editor form shows the effective values for keys
   a file omits. */
class ScenarioBridge : public QObject
{
    Q_OBJECT

public:
    explicit ScenarioBridge(QObject *parent = nullptr);

    /* Parse scenario text. Returns:
       { ok, error, scripted, processCount,
         quantumHi, quantumLo, seed, pIo, pDisk, pTape, pPrinter,
         serviceMin, serviceMax,
         diskMin, diskMax, tapeMin, tapeMax, printerMin, printerMax,
         diskMode, tapeMode, printerMode,          // "concurrent" | "queue"
         processes: [ { arrival, burst, io } ] }   // io = "t:dev[:d[-d]], …"
       On failure only { ok:false, error } is meaningful. */
    Q_INVOKABLE QVariantMap parse(const QString &text) const;

    /* Read + parse the file at path (native path or file:// URL). Adds
       "path" (native) to the returned map. */
    Q_INVOKABLE QVariantMap summarize(const QString &pathOrUrl) const;

    Q_INVOKABLE QString readFile(const QString &pathOrUrl) const;
    Q_INVOKABLE bool    writeFile(const QString &pathOrUrl, const QString &text) const;

    /* Scenarios shipped with the application (the scenarios/ directory next
       to the binary, or in the source tree during development; CUTESIM_SCENARIOS
       overrides). Sorted by file name. Each entry:
       { file, path, title, description }
       title/description come from the leading "#" comment block of the file
       (first line = title, following lines = description). */
    Q_INVOKABLE QVariantList bundledScenarios() const;

    /* file:// URL → native path (FileDialog hands back URLs). */
    Q_INVOKABLE QString toLocalPath(const QString &pathOrUrl) const;
};
