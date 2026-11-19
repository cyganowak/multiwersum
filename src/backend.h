#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Backend;

// One row of the window's list: a saga heading, a section heading, or a
// title. In the list and compare views every row of the catalog is always in
// the model and filters only hide rows, so ticking a box or collapsing a
// section never resets the list and the scroll position stays put. The queue
// view holds only the queued titles, in order.
class RowsModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        KindRole = Qt::UserRole + 1,
        ShownRole,
        SectionIdRole,
        BadgeRole,
        NameRole,
        SubtitleRole,
        OpenRole,
        CountsRole,
        CompleteRole,
        ItemIdRole,
        TitleRole,
        SeasonRole,
        YearRole,
        TypeRole,
        SoonRole,
        DateRole,
        MarksRole,
        QueuedRole,
        QueuePositionRole,
        AllSeenRole,
        HighlightRole,
        SectionNameRole,
    };

    struct Row {
        QString kind;   // "saga", "section", "item"
        int index = -1; // section index for saga/section rows, item index otherwise
    };

    explicit RowsModel(Backend *backend);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<Row> &rows() const { return m_rows; }
    void reset(const QList<Row> &rows);
    void refresh();
    void move(int from, int to);
    void remove(int row);
    void append(const Row &row);

private:
    Backend *m_backend;
    QList<Row> m_rows;
};

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(RowsModel *rows READ rows CONSTANT)
    Q_PROPERTY(QString mode READ mode NOTIFY changed)
    Q_PROPERTY(QString filter READ filter NOTIFY changed)
    Q_PROPERTY(QString query READ query NOTIFY changed)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(int activeProfile READ activeProfile NOTIFY changed)
    Q_PROPERTY(int compareA READ compareA NOTIFY changed)
    Q_PROPERTY(int compareB READ compareB NOTIFY changed)
    Q_PROPERTY(int total READ total CONSTANT)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY changed)
    Q_PROPERTY(int queueSeen READ queueSeen NOTIFY changed)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY changed)
    Q_PROPERTY(bool allCollapsed READ allCollapsed NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(int maxProfiles READ maxProfiles CONSTANT)

public:
    struct Item {
        QString id, title, type, date;
        int season = 0;
        int year = 0;
        int section = -1;
    };
    struct Section {
        QString id, saga, badge, name, subtitle;
        QList<int> items;
    };
    struct Profile {
        QString id, name;
        QSet<QString> watched;
    };

    // An empty path means the user's data directory. Tests pass a temp path.
    explicit Backend(const QString &statePath = {}, QObject *parent = nullptr);

    static QStringList palette();

    RowsModel *rows() { return &m_rows; }
    QString mode() const { return m_mode; }
    QString filter() const { return m_filter; }
    QString query() const { return m_query; }
    QVariantList profiles() const;
    int activeProfile() const { return m_active; }
    int compareA() const { return m_compareA; }
    int compareB() const { return m_compareB; }
    int total() const { return m_items.size(); }
    int queueCount() const { return m_queue.size(); }
    int queueSeen() const;
    QVariantMap stats() const;
    bool allCollapsed() const;
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    bool hasNext() const { return nextUnwatched() >= 0; }
    QString error() const { return m_error; }
    int maxProfiles() const { return 4; }
    QString statePath() const { return m_statePath; }

    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setFilter(const QString &filter);
    Q_INVOKABLE void cycleFilter();
    Q_INVOKABLE void setQuery(const QString &query);
    Q_INVOKABLE void selectProfile(int index);
    Q_INVOKABLE int addProfile();
    Q_INVOKABLE void renameProfile(int index, const QString &name);
    Q_INVOKABLE void removeProfile(int index);
    Q_INVOKABLE void setCompare(int a, int b);
    Q_INVOKABLE void toggleWatched(const QString &itemId, int profile);
    Q_INVOKABLE void toggleQueue(const QString &itemId);
    Q_INVOKABLE void moveInQueue(const QString &itemId, const QString &op);
    Q_INVOKABLE void queueNext();
    Q_INVOKABLE void queueClean();
    Q_INVOKABLE void toggleSection(const QString &sectionId);
    Q_INVOKABLE void setSectionOpen(const QString &sectionId, bool open);
    Q_INVOKABLE void setAllOpen(bool open);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE int nextRow(int from, int direction) const;
    Q_INVOKABLE QString itemAt(int row) const;
    Q_INVOKABLE QString sectionAt(int row) const;

    // Used by the command line and tests.
    bool importFile(const QString &path, QString *message = nullptr);
    bool setWatched(const QString &itemId, int profile, bool watched);
    bool setQueued(const QString &itemId, bool queued);
    int profileByName(const QString &name) const;
    int itemIndex(const QString &id) const { return m_itemIndex.value(id, -1); }
    QJsonObject statusJson() const;
    QJsonObject serialize() const;
    const QList<Item> &items() const { return m_items; }
    const QList<Section> &sections() const { return m_sections; }
    const QList<Profile> &profileList() const { return m_profiles; }
    const QStringList &queue() const { return m_queue; }

    // Row helpers for RowsModel.
    bool rowShown(const RowsModel::Row &row) const;
    bool itemMatches(int item) const;
    bool isSoon(int item) const;
    bool allSeen(const QString &id) const;
    QList<int> shownProfiles() const;
    QVariantList marks(const QString &id) const;
    QVariantList sectionCounts(int section) const;
    bool sectionOpen(int section) const;
    QString highlight(const QString &id) const;

signals:
    void changed();

private:
    void loadCatalog();
    bool load();
    bool restore(const QJsonObject &state);
    void save();
    void remember();
    void touched(bool structural = false);
    void rebuildRows();
    int nextUnwatched() const;

    QString m_statePath;
    QList<Item> m_items;
    QList<Section> m_sections;
    QHash<QString, int> m_itemIndex;

    QList<Profile> m_profiles;
    int m_active = 0;
    int m_compareA = 0;
    int m_compareB = 1;
    QStringList m_queue;
    QSet<QString> m_collapsed;

    QString m_mode = "list";
    QString m_filter = "all";
    QString m_query;
    QString m_error;
    QList<QJsonObject> m_undo, m_redo;

    RowsModel m_rows;
};
