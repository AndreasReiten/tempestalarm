#ifndef PARSEREPLYTASK_H
#define PARSEREPLYTASK_H

#include <QObject>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QRunnable>
#include <QThread>


class ParseReplyTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ParseReplyTask(QSqlDatabase map_db, QSqlDatabase tempest_db, QNetworkReply * p_reply);

signals:
    void finished();
    void mapDataChanged();
    void tempestPrefixDataChanged();
    void tempestSuffixDataChanged();

protected:
    void run();

private:
    QSqlDatabase p_map_db;
    QSqlDatabase p_tempest_affix_db;
    QNetworkReply * p_reply;

    void upsertMap(QString map, int level, QString tempest_prefix, QString tempest_suffix, int tempest_value, int votes);
    void upsertPrefix(QString prefix, QString description);
    void upsertSuffix(QString prefix, QString description);
};

#endif // PARSEREPLYTASK_H
