#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class MultiwersumTests : public QObject {
    Q_OBJECT

private slots:
    void startsWithTwoPeopleAndTheWholeCatalog()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        QCOMPARE(backend.profileList().size(), 2);
        QCOMPARE(backend.total(), 144);
        QVERIFY(backend.error().isEmpty());
    }

    void ticksAreSavedAndReloaded()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("state.json");
        {
            Backend backend(path);
            backend.toggleWatched("iron-man", 0);
            backend.toggleWatched("thor", 1);
            backend.renameProfile(1, "Ania");
            backend.toggleQueue("endgame");
        }
        Backend again(path);
        QVERIFY(again.profileList().at(0).watched.contains("iron-man"));
        QVERIFY(again.profileList().at(1).watched.contains("thor"));
        QCOMPARE(again.profileList().at(1).name, QString("Ania"));
        QCOMPARE(again.queue(), QStringList{"endgame"});
    }

    void undoAndRedoATick()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        backend.toggleWatched("iron-man", 0);
        QVERIFY(backend.canUndo());
        backend.undo();
        QVERIFY(!backend.profileList().at(0).watched.contains("iron-man"));
        backend.redo();
        QVERIFY(backend.profileList().at(0).watched.contains("iron-man"));
    }

    void importsTheWebVersion()
    {
        QTemporaryDir dir;
        QFile web(dir.filePath("web.json"));
        QVERIFY(web.open(QIODevice::WriteOnly));
        web.write(R"({"data": {"profiles": [{"id": "p1", "name": "Alex", "watched": ["iron-man", "bogus"]},
                                            {"id": "p2", "name": "Sam", "watched": ["thor"]}],
                               "active": "p2", "cmp": ["p1", "p2"], "queue": ["endgame", "endgame"]}})");
        web.close();

        Backend backend(dir.filePath("state.json"));
        QString message;
        QVERIFY(backend.importFile(web.fileName(), &message));
        QCOMPARE(backend.profileList().at(1).name, QString("Sam"));
        QCOMPARE(backend.activeProfile(), 1);
        QCOMPARE(backend.profileList().at(0).watched.size(), 1);   // unknown ids are dropped
        QCOMPARE(backend.queue(), QStringList{"endgame"});           // duplicates are dropped
    }

    void importsTheFirstWebVersion()
    {
        QTemporaryDir dir;
        QFile web(dir.filePath("v1.json"));
        QVERIFY(web.open(QIODevice::WriteOnly));
        web.write(R"({"watched": ["iron-man", "thor"]})");
        web.close();
        Backend backend(dir.filePath("state.json"));
        QVERIFY(backend.importFile(web.fileName()));
        QCOMPARE(backend.profileList().size(), 2);
        QCOMPARE(backend.profileList().at(0).watched.size(), 2);
    }

    void damagedSaveIsKeptNotOverwritten()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("state.json");
        QFile bad(path);
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{ not json");
        bad.close();
        Backend backend(path);
        QVERIFY(!backend.error().isEmpty());
        QVERIFY(QFile::exists(path + ".damaged"));
    }

    void queueMovesAndCleans()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        backend.setMode("queue");
        for (const char *id : {"iron-man", "thor", "avengers"})
            backend.toggleQueue(id);
        QCOMPARE(backend.rows()->rowCount(), 3);
        backend.moveInQueue("avengers", "top");
        QCOMPARE(backend.queue().first(), QString("avengers"));
        backend.moveInQueue("avengers", "down");
        QCOMPARE(backend.queue().at(1), QString("avengers"));
        QCOMPARE(backend.itemAt(1), QString("avengers"));   // the model follows the queue
        backend.setWatched("thor", 0, true);
        backend.setWatched("thor", 1, true);
        QCOMPARE(backend.queueSeen(), 1);
        backend.queueClean();
        QCOMPARE(backend.queue(), (QStringList{"iron-man", "avengers"}));
        QCOMPARE(backend.rows()->rowCount(), 2);
    }

    void nextUnwatchedSkipsQueuedAndSeen()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        backend.setWatched("iron-man", 0, true);
        backend.setWatched("iron-man", 1, true);
        backend.queueNext();
        QCOMPARE(backend.queue(), QStringList{"incredible-hulk"});
        backend.queueNext();
        QCOMPARE(backend.queue().last(), QString("iron-man-2"));
    }

    void compareFiltersCountDifferences()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        backend.setWatched("iron-man", 0, true);
        backend.setWatched("thor", 1, true);
        backend.setWatched("avengers", 0, true);
        backend.setWatched("avengers", 1, true);
        backend.setMode("compare");
        const auto stats = backend.stats();
        QCOMPARE(stats.value("a").toInt(), 1);
        QCOMPARE(stats.value("b").toInt(), 1);
        QCOMPARE(stats.value("both").toInt(), 1);
        QCOMPARE(stats.value("none").toInt(), 141);
        backend.setFilter("a");
        int shown = 0;
        for (int r = 0; r < backend.rows()->rowCount(); ++r)
            if (backend.rows()->data(backend.rows()->index(r), RowsModel::ShownRole).toBool()
                && !backend.itemAt(r).isEmpty())
                ++shown;
        QCOMPARE(shown, 1);
    }

    void searchAndCollapseHideRowsWithoutResetting()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        const int rows = backend.rows()->rowCount();
        QSignalSpy resets(backend.rows(), &QAbstractItemModel::modelReset);
        backend.setSectionOpen("f1", false);
        backend.setQuery("spider");
        QCOMPARE(backend.rows()->rowCount(), rows);
        QCOMPARE(resets.count(), 0);
        QCOMPARE(backend.itemAt(backend.nextRow(-1, 1)), QString("homecoming"));
    }

    void removingAPersonKeepsTheOthers()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"));
        backend.addProfile();
        backend.renameProfile(2, "Sam");
        backend.setWatched("thor", 2, true);
        backend.removeProfile(0);
        QCOMPARE(backend.profileList().size(), 2);
        QCOMPARE(backend.profileList().at(1).name, QString("Sam"));
        QVERIFY(backend.profileList().at(1).watched.contains("thor"));
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void polishCatalogCoversTheInterface()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pl\n");
        Translator translator(dir.path() + "/language");
        QCOMPARE(translator.language(), QString("pl"));
        QCOMPARE(translator.translate("Main", "Phase One"), QString("Faza pierwsza"));
        QCOMPARE(translator.translate("Main", "%n in the queue", nullptr, 3), QString("%n w kolejce"));
        QCOMPARE(translator.translate("Main", "%n seen by everyone", nullptr, 5), QString("%n obejrzanych przez wszystkich"));
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(MultiwersumTests)
#include "multiwersum_tests.moc"
