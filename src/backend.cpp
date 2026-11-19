#include "backend.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

const QStringList kFilters{"all", "film", "series", "todo"};
const QStringList kCompareFilters{"all", "a", "b", "both", "none"};
constexpr int kUndoLimit = 200;

QString newProfileId()
{
    return "p" + QString::number(QRandomGenerator::global()->generate() % 0xFFFFFF, 36);
}

QString initialOf(const QString &name)
{
    const QString trimmed = name.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("?") : trimmed.left(1).toUpper();
}

} // namespace

// ------------------------------------------------------------------ RowsModel

RowsModel::RowsModel(Backend *backend) : QAbstractListModel(backend), m_backend(backend) {}

int RowsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QHash<int, QByteArray> RowsModel::roleNames() const
{
    return {
        {KindRole, "kind"},
        {ShownRole, "shown"},
        {SectionIdRole, "sectionId"},
        {BadgeRole, "badge"},
        {NameRole, "name"},
        {SubtitleRole, "subtitle"},
        {OpenRole, "open"},
        {CountsRole, "counts"},
        {CompleteRole, "complete"},
        {ItemIdRole, "itemId"},
        {TitleRole, "title"},
        {SeasonRole, "season"},
        {YearRole, "year"},
        {TypeRole, "type"},
        {SoonRole, "soon"},
        {DateRole, "date"},
        {MarksRole, "marks"},
        {QueuedRole, "queued"},
        {QueuePositionRole, "queuePosition"},
        {AllSeenRole, "allSeen"},
        {HighlightRole, "highlight"},
        {SectionNameRole, "sectionName"},
    };
}

QVariant RowsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows.at(index.row());
    const auto &sections = m_backend->sections();
    const auto &items = m_backend->items();

    if (role == KindRole)
        return row.kind == "item" && m_backend->mode() == "queue" ? QStringLiteral("queued") : row.kind;
    if (role == ShownRole)
        return m_backend->rowShown(row);

    if (row.kind != "item") {
        const auto &section = sections.at(row.index);
        switch (role) {
        case SectionIdRole: return section.id;
        case BadgeRole: return section.badge;
        case NameRole: return row.kind == "saga" ? section.saga : section.name;
        case SubtitleRole: return section.subtitle;
        case OpenRole: return m_backend->sectionOpen(row.index);
        case CountsRole: return m_backend->sectionCounts(row.index);
        case CompleteRole: {
            const auto counts = m_backend->sectionCounts(row.index);
            if (counts.isEmpty())
                return false;
            for (const auto &c : counts)
                if (c.toMap().value("count").toInt() < section.items.size())
                    return false;
            return true;
        }
        default: return {};
        }
    }

    const auto &item = items.at(row.index);
    switch (role) {
    case SectionIdRole: return sections.at(item.section).id;
    case SectionNameRole: return sections.at(item.section).name;
    case ItemIdRole: return item.id;
    case TitleRole: return item.title;
    case SeasonRole: return item.season;
    case YearRole: return item.year;
    case TypeRole: return item.type;
    case SoonRole: return m_backend->isSoon(row.index);
    case DateRole:
        return item.date.isEmpty() ? QString()
                                   : QLocale().toString(QDate::fromString(item.date, Qt::ISODate), QLocale::ShortFormat);
    case MarksRole: return m_backend->marks(item.id);
    case QueuedRole: return m_backend->queue().contains(item.id);
    case QueuePositionRole: return m_backend->queue().indexOf(item.id) + 1;
    case AllSeenRole: return m_backend->allSeen(item.id);
    case HighlightRole: return m_backend->highlight(item.id);
    default: return {};
    }
}

void RowsModel::reset(const QList<Row> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

void RowsModel::refresh()
{
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(m_rows.size() - 1));
}

void RowsModel::move(int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= m_rows.size() || to >= m_rows.size())
        return;
    // Qt's destination index counts the row being moved when moving down.
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_rows.move(from, to);
    endMoveRows();
}

void RowsModel::remove(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    beginRemoveRows({}, row, row);
    m_rows.removeAt(row);
    endRemoveRows();
}

void RowsModel::append(const Row &row)
{
    beginInsertRows({}, m_rows.size(), m_rows.size());
    m_rows.append(row);
    endInsertRows();
}

// -------------------------------------------------------------------- Backend

Backend::Backend(const QString &statePath, QObject *parent)
    : QObject(parent), m_statePath(statePath), m_rows(this)
{
    if (m_statePath.isEmpty())
        m_statePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/multiwersum/state.json";
    loadCatalog();
    if (!load()) {
        m_profiles = {{"p1", tr("Me"), {}}, {"p2", tr("Person 2"), {}}};
        m_compareA = 0;
        m_compareB = 1;
    }
    rebuildRows();
}

QStringList Backend::palette()
{
    // Fixed per-profile colors rather than theme colors: a profile must keep
    // its color across theme switches, and monochrome themes have no hues to
    // tell four people apart.
    return {"#e5484d", "#3e7bfa", "#f5a524", "#a371f7"};
}

void Backend::loadCatalog()
{
    QFile file(":/catalog.json");
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = tr("The built-in catalog could not be read.");
        return;
    }
    const auto sections = QJsonDocument::fromJson(file.readAll()).object().value("sections").toArray();
    for (const auto &sv : sections) {
        const auto so = sv.toObject();
        Section section{so.value("id").toString(), so.value("saga").toString(), so.value("badge").toString(),
                        so.value("name").toString(), so.value("subtitle").toString(), {}};
        const int sectionIndex = m_sections.size();
        for (const auto &iv : so.value("items").toArray()) {
            const auto io = iv.toObject();
            Item item{io.value("id").toString(), io.value("title").toString(), io.value("type").toString(),
                      io.value("date").toString(), io.value("season").toInt(), io.value("year").toInt(), sectionIndex};
            m_itemIndex.insert(item.id, m_items.size());
            section.items.append(m_items.size());
            m_items.append(item);
        }
        m_sections.append(section);
    }
}

// The state file is the same shape the web version saved, so its data can be
// imported as is: {profiles: [{id, name, watched: []}], active, cmp, queue}.
// The very first web version saved only {watched: []}, which becomes "Me".
bool Backend::restore(const QJsonObject &state)
{
    QList<Profile> profiles;
    const auto list = state.value("profiles").toArray();
    for (const auto &pv : list) {
        if (profiles.size() >= maxProfiles())
            break;
        const auto po = pv.toObject();
        Profile p{po.value("id").toString(), po.value("name").toString().left(24), {}};
        if (p.id.isEmpty())
            p.id = newProfileId();
        if (p.name.trimmed().isEmpty())
            p.name = tr("Profile");
        for (const auto &w : po.value("watched").toArray())
            if (m_itemIndex.contains(w.toString()))
                p.watched.insert(w.toString());
        profiles.append(p);
    }
    if (profiles.isEmpty() && state.value("watched").isArray()) {
        Profile me{"p1", tr("Me"), {}};
        for (const auto &w : state.value("watched").toArray())
            if (m_itemIndex.contains(w.toString()))
                me.watched.insert(w.toString());
        profiles = {me, {"p2", tr("Person 2"), {}}};
    }
    if (profiles.isEmpty())
        return false;

    auto indexOfId = [&](const QString &id) {
        for (int i = 0; i < profiles.size(); ++i)
            if (profiles[i].id == id)
                return i;
        return -1;
    };
    m_profiles = profiles;
    m_active = qMax(0, indexOfId(state.value("active").toString()));
    const auto cmp = state.value("cmp").toArray();
    m_compareA = cmp.size() > 0 ? indexOfId(cmp.at(0).toString()) : -1;
    m_compareB = cmp.size() > 1 ? indexOfId(cmp.at(1).toString()) : -1;
    if (m_compareA < 0 || m_compareB < 0 || m_compareA == m_compareB) {
        m_compareA = 0;
        m_compareB = m_profiles.size() > 1 ? 1 : 0;
    }
    m_queue.clear();
    for (const auto &q : state.value("queue").toArray())
        if (m_itemIndex.contains(q.toString()) && !m_queue.contains(q.toString()))
            m_queue.append(q.toString());
    m_collapsed.clear();
    for (const auto &c : state.value("collapsed").toArray())
        m_collapsed.insert(c.toString());
    return true;
}

QJsonObject Backend::serialize() const
{
    QJsonArray profiles;
    for (const auto &p : m_profiles) {
        QJsonArray watched;
        // Catalog order keeps the file stable and readable between saves.
        for (const auto &item : m_items)
            if (p.watched.contains(item.id))
                watched.append(item.id);
        profiles.append(QJsonObject{{"id", p.id}, {"name", p.name}, {"watched", watched}});
    }
    QJsonArray collapsed;
    for (const auto &s : m_sections)
        if (m_collapsed.contains(s.id))
            collapsed.append(s.id);
    return {
        {"version", 1},
        {"profiles", profiles},
        {"active", m_profiles.value(m_active).id},
        {"cmp", QJsonArray{m_profiles.value(m_compareA).id, m_profiles.value(m_compareB).id}},
        {"queue", QJsonArray::fromStringList(m_queue)},
        {"collapsed", collapsed},
    };
}

bool Backend::load()
{
    QFile file(m_statePath);
    if (!file.exists())
        return false;
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = tr("Could not read %1.").arg(m_statePath);
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        // Keep the damaged file for the user instead of overwriting it.
        QFile::copy(m_statePath, m_statePath + ".damaged");
        m_error = tr("Your saved list was damaged; a copy is in %1.").arg(m_statePath + ".damaged");
        return false;
    }
    return restore(document.object());
}

void Backend::save()
{
    QDir().mkpath(QFileInfo(m_statePath).absolutePath());
    // QSaveFile writes beside the target and renames, so a crash mid-save
    // never leaves a half-written list.
    QSaveFile file(m_statePath);
    const QString previous = m_error;
    if (file.open(QIODevice::WriteOnly)
        && file.write(QJsonDocument(serialize()).toJson(QJsonDocument::Indented)) >= 0
        && file.commit()) {
        if (!m_error.isEmpty() && m_error.startsWith(tr("Could not save")))
            m_error.clear();
    } else {
        m_error = tr("Could not save to %1: %2").arg(m_statePath, file.errorString());
    }
    if (m_error != previous)
        emit changed();
}

void Backend::remember()
{
    m_undo.append(serialize());
    if (m_undo.size() > kUndoLimit)
        m_undo.removeFirst();
    m_redo.clear();
}

void Backend::touched(bool structural)
{
    save();
    if (structural)
        rebuildRows();
    else
        m_rows.refresh();
    emit changed();
}

void Backend::rebuildRows()
{
    QList<RowsModel::Row> rows;
    if (m_mode == "queue") {
        for (const auto &id : m_queue)
            rows.append({"item", m_itemIndex.value(id)});
    } else {
        QString saga;
        for (int s = 0; s < m_sections.size(); ++s) {
            if (m_sections[s].saga != saga) {
                saga = m_sections[s].saga;
                rows.append({"saga", s});
            }
            rows.append({"section", s});
            for (int i : m_sections[s].items)
                rows.append({"item", i});
        }
    }
    m_rows.reset(rows);
}

// ----------------------------------------------------------------- queries

QVariantList Backend::profiles() const
{
    QVariantList list;
    const auto colors = palette();
    for (int i = 0; i < m_profiles.size(); ++i) {
        const auto &p = m_profiles[i];
        list.append(QVariantMap{
            {"name", p.name},
            {"initial", initialOf(p.name)},
            {"color", colors.at(i % colors.size())},
            {"count", p.watched.size()},
            {"percent", m_items.isEmpty() ? 0 : qRound(100.0 * p.watched.size() / m_items.size())},
        });
    }
    return list;
}

int Backend::queueSeen() const
{
    int n = 0;
    for (const auto &id : m_queue)
        if (allSeen(id))
            ++n;
    return n;
}

QVariantMap Backend::stats() const
{
    int both = 0, a = 0, b = 0, none = 0;
    const auto &A = m_profiles.value(m_compareA).watched, &B = m_profiles.value(m_compareB).watched;
    for (const auto &item : m_items) {
        const bool x = A.contains(item.id), y = B.contains(item.id);
        if (x && y) ++both;
        else if (x) ++a;
        else if (y) ++b;
        else ++none;
    }
    return {{"both", both}, {"a", a}, {"b", b}, {"none", none}};
}

bool Backend::allCollapsed() const
{
    for (const auto &s : m_sections)
        if (!m_collapsed.contains(s.id))
            return false;
    return true;
}

bool Backend::isSoon(int item) const
{
    const auto &it = m_items.at(item);
    const QDate today = QDate::currentDate();
    if (!it.date.isEmpty())
        return QDate::fromString(it.date, Qt::ISODate) > today;
    return it.year > today.year();
}

bool Backend::allSeen(const QString &id) const
{
    for (const auto &p : m_profiles)
        if (!p.watched.contains(id))
            return false;
    return !m_profiles.isEmpty();
}

QList<int> Backend::shownProfiles() const
{
    if (m_mode == "compare")
        return {m_compareA, m_compareB};
    if (m_mode == "queue") {
        QList<int> all;
        for (int i = 0; i < m_profiles.size(); ++i)
            all.append(i);
        return all;
    }
    return {m_active};
}

QVariantList Backend::marks(const QString &id) const
{
    QVariantList list;
    const auto colors = palette();
    for (int i : shownProfiles()) {
        const auto &p = m_profiles.at(i);
        list.append(QVariantMap{{"profile", i}, {"on", p.watched.contains(id)}, {"color", colors.at(i % colors.size())},
                                {"initial", initialOf(p.name)}, {"name", p.name}});
    }
    return list;
}

QVariantList Backend::sectionCounts(int section) const
{
    QVariantList list;
    const auto colors = palette();
    const auto &s = m_sections.at(section);
    const QList<int> who = m_mode == "compare" ? QList<int>{m_compareA, m_compareB} : QList<int>{m_active};
    for (int i : who) {
        int count = 0;
        for (int item : s.items)
            if (m_profiles.at(i).watched.contains(m_items.at(item).id))
                ++count;
        list.append(QVariantMap{{"count", count}, {"total", s.items.size()},
                                {"color", colors.at(i % colors.size())}, {"initial", initialOf(m_profiles.at(i).name)}});
    }
    return list;
}

bool Backend::sectionOpen(int section) const
{
    // Searching or filtering opens every section, so a match is never hidden
    // inside a collapsed one.
    if (!m_query.isEmpty() || m_filter != "all")
        return true;
    return !m_collapsed.contains(m_sections.at(section).id);
}

QString Backend::highlight(const QString &id) const
{
    if (m_mode != "compare")
        return {};
    const bool a = m_profiles.at(m_compareA).watched.contains(id), b = m_profiles.at(m_compareB).watched.contains(id);
    const auto colors = palette();
    if (a && !b) return colors.at(m_compareA % colors.size());
    if (b && !a) return colors.at(m_compareB % colors.size());
    return {};
}

bool Backend::itemMatches(int index) const
{
    const auto &item = m_items.at(index);
    if (!m_query.isEmpty() && !item.title.contains(m_query, Qt::CaseInsensitive))
        return false;
    if (m_mode == "queue")
        return true;
    if (m_mode == "list") {
        if (m_filter == "film") return item.type == "film" || item.type == "special";
        if (m_filter == "series") return item.type == "series" || item.type == "animated";
        if (m_filter == "todo") return !m_profiles.at(m_active).watched.contains(item.id);
        return true;
    }
    const bool a = m_profiles.at(m_compareA).watched.contains(item.id);
    const bool b = m_profiles.at(m_compareB).watched.contains(item.id);
    if (m_filter == "both") return a && b;
    if (m_filter == "a") return a && !b;
    if (m_filter == "b") return b && !a;
    if (m_filter == "none") return !a && !b;
    return true;
}

bool Backend::rowShown(const RowsModel::Row &row) const
{
    if (row.kind == "item") {
        if (m_mode == "queue")
            return itemMatches(row.index);
        return itemMatches(row.index) && sectionOpen(m_items.at(row.index).section);
    }
    auto sectionHasMatch = [&](int s) {
        for (int i : m_sections.at(s).items)
            if (itemMatches(i))
                return true;
        return false;
    };
    if (row.kind == "section")
        return sectionHasMatch(row.index);
    // A saga heading shows while any section under it does.
    for (int s = row.index; s < m_sections.size() && m_sections[s].saga == m_sections[row.index].saga; ++s)
        if (sectionHasMatch(s))
            return true;
    return false;
}

int Backend::nextRow(int from, int direction) const
{
    const auto &rows = m_rows.rows();
    for (int r = from + direction; r >= 0 && r < rows.size(); r += direction)
        if (rows[r].kind == "item" && rowShown(rows[r]))
            return r;
    return -1;
}

QString Backend::itemAt(int row) const
{
    const auto &rows = m_rows.rows();
    if (row < 0 || row >= rows.size() || rows[row].kind != "item")
        return {};
    return m_items.at(rows[row].index).id;
}

QString Backend::sectionAt(int row) const
{
    const auto &rows = m_rows.rows();
    if (row < 0 || row >= rows.size())
        return {};
    const auto &r = rows[row];
    return m_sections.at(r.kind == "item" ? m_items.at(r.index).section : r.index).id;
}

int Backend::nextUnwatched() const
{
    // The next MCU title in release order that not everyone has seen yet,
    // skipping titles that haven't premiered.
    for (const auto &s : m_sections) {
        if (!(s.id.size() == 2 && s.id.startsWith('f')))
            continue;
        for (int i : s.items)
            if (!isSoon(i) && !m_queue.contains(m_items[i].id) && !allSeen(m_items[i].id))
                return i;
    }
    return -1;
}

int Backend::profileByName(const QString &name) const
{
    for (int i = 0; i < m_profiles.size(); ++i)
        if (m_profiles[i].name.compare(name, Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

QJsonObject Backend::statusJson() const
{
    QJsonArray profiles;
    for (const auto &p : m_profiles)
        profiles.append(QJsonObject{{"name", p.name}, {"watched", p.watched.size()}});
    return {{"total", m_items.size()}, {"profiles", profiles}, {"queue", QJsonArray::fromStringList(m_queue)},
            {"state", m_statePath}};
}

// ----------------------------------------------------------------- actions

void Backend::setMode(const QString &mode)
{
    if (mode == m_mode || !(mode == "list" || mode == "compare" || mode == "queue"))
        return;
    if (mode == "compare" && m_profiles.size() < 2)
        return;
    m_mode = mode;
    m_filter = "all";
    rebuildRows();
    emit changed();
}

void Backend::setFilter(const QString &filter)
{
    const auto &valid = m_mode == "compare" ? kCompareFilters : kFilters;
    if (!valid.contains(filter) || filter == m_filter)
        return;
    m_filter = filter;
    m_rows.refresh();
    emit changed();
}

void Backend::cycleFilter()
{
    if (m_mode == "queue")
        return;
    const auto &valid = m_mode == "compare" ? kCompareFilters : kFilters;
    setFilter(valid.at((valid.indexOf(m_filter) + 1) % valid.size()));
}

void Backend::setQuery(const QString &query)
{
    const QString q = query.trimmed();
    if (q == m_query)
        return;
    m_query = q;
    m_rows.refresh();
    emit changed();
}

void Backend::selectProfile(int index)
{
    if (index < 0 || index >= m_profiles.size() || index == m_active)
        return;
    m_active = index;
    save();
    m_rows.refresh();
    emit changed();
}

int Backend::addProfile()
{
    if (m_profiles.size() >= maxProfiles())
        return -1;
    remember();
    m_profiles.append({newProfileId(), tr("Person %1").arg(m_profiles.size() + 1), {}});
    m_active = m_profiles.size() - 1;
    touched();
    return m_active;
}

void Backend::renameProfile(int index, const QString &name)
{
    const QString n = name.trimmed().left(24);
    if (index < 0 || index >= m_profiles.size() || n.isEmpty() || n == m_profiles[index].name)
        return;
    remember();
    m_profiles[index].name = n;
    touched();
}

void Backend::removeProfile(int index)
{
    if (index < 0 || index >= m_profiles.size() || m_profiles.size() < 2)
        return;
    remember();
    const QString activeId = m_profiles[m_active].id;
    const QString aId = m_profiles[m_compareA].id, bId = m_profiles[m_compareB].id;
    m_profiles.removeAt(index);
    auto find = [&](const QString &id) {
        for (int i = 0; i < m_profiles.size(); ++i)
            if (m_profiles[i].id == id)
                return i;
        return -1;
    };
    m_active = qMax(0, find(activeId));
    m_compareA = find(aId);
    m_compareB = find(bId);
    if (m_compareA < 0 || m_compareB < 0 || m_compareA == m_compareB) {
        m_compareA = 0;
        m_compareB = m_profiles.size() > 1 ? 1 : 0;
    }
    if (m_profiles.size() < 2 && m_mode == "compare") {
        m_mode = "list";
        m_filter = "all";
        touched(true);
        return;
    }
    touched(m_mode == "queue");
}

void Backend::setCompare(int a, int b)
{
    if (a < 0 || b < 0 || a >= m_profiles.size() || b >= m_profiles.size())
        return;
    // Picking the same person on both sides swaps them.
    if (a == b)
        b = (a == m_compareA) ? m_compareB : m_compareA;
    if (a == b || (a == m_compareA && b == m_compareB))
        return;
    m_compareA = a;
    m_compareB = b;
    m_filter = "all";
    save();
    m_rows.refresh();
    emit changed();
}

bool Backend::setWatched(const QString &itemId, int profile, bool watched)
{
    if (!m_itemIndex.contains(itemId) || profile < 0 || profile >= m_profiles.size())
        return false;
    if (m_profiles[profile].watched.contains(itemId) == watched)
        return true;
    remember();
    if (watched)
        m_profiles[profile].watched.insert(itemId);
    else
        m_profiles[profile].watched.remove(itemId);
    touched();
    return true;
}

void Backend::toggleWatched(const QString &itemId, int profile)
{
    if (profile < 0 || profile >= m_profiles.size())
        return;
    setWatched(itemId, profile, !m_profiles[profile].watched.contains(itemId));
}

bool Backend::setQueued(const QString &itemId, bool queued)
{
    if (!m_itemIndex.contains(itemId))
        return false;
    const int at = m_queue.indexOf(itemId);
    if ((at >= 0) == queued)
        return true;
    remember();
    if (queued) {
        m_queue.append(itemId);
        if (m_mode == "queue")
            m_rows.append({"item", m_itemIndex.value(itemId)});
    } else {
        m_queue.removeAt(at);
        if (m_mode == "queue")
            m_rows.remove(at);
    }
    touched();
    return true;
}

void Backend::toggleQueue(const QString &itemId)
{
    setQueued(itemId, !m_queue.contains(itemId));
}

void Backend::moveInQueue(const QString &itemId, const QString &op)
{
    const int at = m_queue.indexOf(itemId);
    if (at < 0)
        return;
    if (op == "remove") {
        setQueued(itemId, false);
        return;
    }
    const int to = op == "top" ? 0 : op == "up" ? at - 1 : op == "down" ? at + 1 : at;
    if (to < 0 || to >= m_queue.size() || to == at)
        return;
    remember();
    m_queue.move(at, to);
    if (m_mode == "queue")
        m_rows.move(at, to);
    touched();
}

void Backend::queueNext()
{
    const int next = nextUnwatched();
    if (next >= 0)
        setQueued(m_items[next].id, true);
}

void Backend::queueClean()
{
    QStringList kept;
    for (const auto &id : m_queue)
        if (!allSeen(id))
            kept.append(id);
    if (kept.size() == m_queue.size())
        return;
    remember();
    m_queue = kept;
    touched(m_mode == "queue");
}

void Backend::setSectionOpen(const QString &sectionId, bool open)
{
    if (m_collapsed.contains(sectionId) == !open)
        return;
    if (open)
        m_collapsed.remove(sectionId);
    else
        m_collapsed.insert(sectionId);
    save();
    m_rows.refresh();
    emit changed();
}

void Backend::toggleSection(const QString &sectionId)
{
    setSectionOpen(sectionId, m_collapsed.contains(sectionId));
}

void Backend::setAllOpen(bool open)
{
    if (open)
        m_collapsed.clear();
    else
        for (const auto &s : m_sections)
            m_collapsed.insert(s.id);
    save();
    m_rows.refresh();
    emit changed();
}

void Backend::undo()
{
    if (m_undo.isEmpty())
        return;
    m_redo.append(serialize());
    // Collapsed sections are a view setting, not an edit, so undo keeps them.
    const auto collapsed = m_collapsed;
    restore(m_undo.takeLast());
    m_collapsed = collapsed;
    if (m_profiles.size() < 2 && m_mode == "compare")
        m_mode = "list";
    touched(true);
}

void Backend::redo()
{
    if (m_redo.isEmpty())
        return;
    m_undo.append(serialize());
    // Collapsed sections are a view setting, not an edit, so undo keeps them.
    const auto collapsed = m_collapsed;
    restore(m_redo.takeLast());
    m_collapsed = collapsed;
    if (m_profiles.size() < 2 && m_mode == "compare")
        m_mode = "list";
    touched(true);
}

bool Backend::importFile(const QString &path, QString *message)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (message) *message = tr("Could not read %1.").arg(path);
        return false;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    QJsonObject state = document.object();
    // A database export wraps the saved fields in "data".
    if (state.value("data").isObject() && !state.contains("profiles") && !state.contains("watched"))
        state = state.value("data").toObject();
    remember();
    if (!restore(state)) {
        m_undo.removeLast();
        if (message) *message = tr("%1 has no profiles or watched titles.").arg(path);
        return false;
    }
    touched(true);
    if (message) *message = tr("Imported %1.").arg(path);
    return true;
}
