// multiwersum — tick off every Marvel film and series, compare what each
// person has seen, and keep a shared queue of what to watch next.

#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
#include <QUrl>

#include "backend.h"
#include "theme.h"
#include "translator.h"

namespace {

const char *kUsage = R"(Usage: multiwersum [command]

  open                         Open the window (the default)
  status [--json]              Watched counts per profile and the queue
  list [--json]                Every title with its id
  mark <id> [--profile NAME]   Mark a title as watched (--unmark to undo)
  queue add|remove <id>        Add a title to the queue or take it off
  import <file.json>           Replace the saved list with a JSON export

Titles are named by id, as `multiwersum list` prints them.
)";

int fail(const QString &message)
{
    QTextStream(stderr) << message << "\n";
    return 1;
}

void out(const QString &text)
{
    QTextStream(stdout) << text << "\n";
}

int runCommand(const QStringList &args)
{
    const QString command = args.value(0);
    const bool json = args.contains("--json");
    Backend backend;
    if (!backend.error().isEmpty())
        return fail(backend.error());

    if (command == "status") {
        const auto status = backend.statusJson();
        if (json) {
            out(QString::fromUtf8(QJsonDocument(status).toJson(QJsonDocument::Compact)));
            return 0;
        }
        for (const auto &p : status.value("profiles").toArray())
            out(QString("%1: %2 / %3").arg(p.toObject().value("name").toString())
                    .arg(p.toObject().value("watched").toInt()).arg(backend.total()));
        out(QString("queue: %1").arg(backend.queueCount()));
        for (const auto &id : backend.queue())
            out("  " + backend.items().at(backend.itemIndex(id)).title + "  (" + id + ")");
        return 0;
    }

    if (command == "list") {
        QJsonArray all;
        for (const auto &item : backend.items()) {
            const auto &section = backend.sections().at(item.section);
            if (json) {
                QJsonObject o{{"id", item.id}, {"title", item.title}, {"year", item.year},
                              {"type", item.type}, {"section", section.id}};
                if (item.season)
                    o.insert("season", item.season);
                all.append(o);
            } else {
                out(QString("%1\t%2%3 (%4)").arg(item.id, item.title,
                    item.season ? QString(" S%1").arg(item.season) : QString()).arg(item.year));
            }
        }
        if (json)
            out(QString::fromUtf8(QJsonDocument(all).toJson(QJsonDocument::Compact)));
        return 0;
    }

    if (command == "mark") {
        const QString id = args.value(1);
        if (backend.itemIndex(id) < 0)
            return fail(QString("Unknown title id: %1 (see `multiwersum list`)").arg(id));
        int profile = backend.activeProfile();
        const int at = args.indexOf("--profile");
        if (at >= 0) {
            profile = backend.profileByName(args.value(at + 1));
            if (profile < 0)
                return fail(QString("No profile named %1").arg(args.value(at + 1)));
        }
        backend.setWatched(id, profile, !args.contains("--unmark"));
        return backend.error().isEmpty() ? 0 : fail(backend.error());
    }

    if (command == "queue") {
        const QString op = args.value(1), id = args.value(2);
        if ((op != "add" && op != "remove") || backend.itemIndex(id) < 0)
            return fail("Usage: multiwersum queue add|remove <id>");
        backend.setQueued(id, op == "add");
        return backend.error().isEmpty() ? 0 : fail(backend.error());
    }

    if (command == "import") {
        QString message;
        if (!backend.importFile(args.value(1), &message))
            return fail(message);
        out(message);
        return backend.error().isEmpty() ? 0 : fail(backend.error());
    }

    return fail(QString("Unknown command: %1\n\n%2").arg(command, kUsage));
}

} // namespace

int main(int argc, char *argv[])
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));
    // A launcher may pass a file path through %f; anything that isn't a
    // command opens the window.
    const QString command = args.value(0);
    const bool gui = command.isEmpty() || command == "open" || command.startsWith('/');

    if (command == "help" || command == "--help" || command == "-h") {
        QTextStream(stdout) << kUsage;
        return 0;
    }

    if (!gui) {
        // Commands need no display, so agents and scripts can drive the list.
        qputenv("QT_QPA_PLATFORM", "offscreen");
        QCoreApplication app(argc, argv);
        app.setApplicationName("multiwersum");
        return runCommand(args);
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("multiwersum");
    app.setApplicationDisplayName("Multiwersum");
    app.setApplicationVersion("0.1.0");

    // Matches the window to multiwersum.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName("multiwersum");
    app.setWindowIcon(QIcon::fromTheme("multiwersum"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
