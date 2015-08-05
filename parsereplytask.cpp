#include "parsereplytask.h"
#include "guncompress.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSqlError>

ParseReplyTask::ParseReplyTask(QSqlDatabase * map_db, QSqlDatabase *tempest_db, QNetworkReply * reply)
{
    p_map_db = map_db;
    p_tempest_affix_db = tempest_db;
    p_reply = reply;
}

void ParseReplyTask::run()
{
    /* Parse reply */
    QByteArray bytes = p_reply->readAll();
    p_reply->deleteLater();

    // Return if the reply is truncated
    if (bytes.size() <= 4)
    {
        return;
    }

    // Decompress reply
    QString str = QString(gUncompress(bytes)).simplified();

    QRegularExpression rx_map(QString("<a onclick=\"downvote.*?<td>\\s*(.*?)")+
                              QString("\\s*\\((\\d+)\\)")+
                              QString(".*?(?:<span class\=\'votes\'>(\\d+)<\/span> )*<\/td>\\s*<td>(.*?)")+
                              QString("(?: of (.*?))*")+
                              QString("\\s*<\/td>\\s*<td class='hide-on-small-and-down\'>(.*?)")+
                              QString("\\.(.*?)<\/td>"));

    int rx_index = 0;

    while (rx_index >= 0)
    {
        QRegularExpressionMatch rx_map_match = rx_map.match(str,rx_index);
        rx_index = rx_map_match.capturedEnd();

        if (rx_index > 0)
        {
            QString map(rx_map_match.captured(1));
            int level = rx_map_match.captured(2).toInt();
            int votes = rx_map_match.captured(3).toInt();
            QString tempest_prefix(rx_map_match.captured(4));
            QString tempest_suffix(rx_map_match.captured(5));
            QString tempest_prefix_description(rx_map_match.captured(6));
            QString tempest_suffix_description(rx_map_match.captured(7));

            if (map != "")
            {
                if (tempest_prefix == "None") upsertMap(map, level, "No tempest", "", -20, votes);
                else if (tempest_prefix == "Unknown Tempest") upsertMap(map, level, "Unknown", "", -10, 0);
                else
                {
                    // Compute tempest affix value
                    int prefix_value;
                    int suffix_value;


                    QSqlQuery query(*p_tempest_affix_db);
                    query.prepare("SELECT Value FROM Prefix WHERE Name=:Name;");
                    query.bindValue(":Name", tempest_prefix);
                    if (!query.exec())
                    {
                        qDebug() << "SQL query failed ( "+query.executedQuery()+"): "+query.lastError().text();
                    }

                    if(query.next())
                    {
                        // Get a gzipped reply from the server
                        prefix_value = query.value(0).toInt();
                    }
                    else
                    {
                        prefix_value = 0;
                    }

                    query.prepare("SELECT Value FROM Suffix WHERE Name=:Name;");
                    query.bindValue(":Name", tempest_suffix);
                    if (!query.exec())
                    {
                        qDebug() << "SQL query failed ( "+query.executedQuery()+"): "+query.lastError().text();
                    }

                    if(query.next())
                    {
                        // Get a gzipped reply from the server
                        suffix_value = query.value(0).toInt();
                    }
                    else
                    {
                        suffix_value = 0;
                    }

                    int tempest_value = prefix_value + suffix_value;

                    if (tempest_prefix != "") upsertPrefix(tempest_prefix, tempest_prefix_description);
                    if (tempest_suffix != "") upsertSuffix(tempest_suffix, tempest_suffix_description);
                    upsertMap(map, level, tempest_prefix, tempest_suffix, tempest_value, votes);
                }
            }
        }
    }
    emit mapDataChanged();
    emit finished();
}

void ParseReplyTask::upsertMap(QString map, int level, QString tempest_prefix, QString tempest_suffix, int tempest_value, int votes)
{
    QSqlQuery query(*p_map_db);
    query.prepare("INSERT OR REPLACE INTO Maps (Name, Level, TempestPrefix, TempestSuffix, TempestValue, Votes) "
                  "VALUES (:Name,:Level,:TempestPrefix,:TempestSuffix,:TempestValue,:Votes);");
    query.bindValue(":Name", map);
    query.bindValue(":Level", level);
    query.bindValue(":TempestPrefix", tempest_prefix);
    query.bindValue(":TempestSuffix", tempest_suffix);
    query.bindValue(":TempestValue", tempest_value);
    query.bindValue(":Votes", votes);

    if (!query.exec())
    {
        qDebug() << "SQL query failed ( "+query.executedQuery()+"): "+query.lastError().text();
    }

//    if (query.numRowsAffected() > 0) emit mapDataChanged();
}

void ParseReplyTask::upsertPrefix(QString prefix, QString description)
{
    QSqlQuery query(*p_tempest_affix_db);
    query.prepare("INSERT OR IGNORE INTO Prefix (Name, Value, Description) "
                  "VALUES (:Name,(SELECT Value FROM Prefix WHERE Name=:Name),:Description);");
    query.bindValue(":Name", prefix);
    query.bindValue(":Description", description);

    if (!query.exec())
    {
        qDebug() << "SQL query failed ( "+query.executedQuery()+"): "+query.lastError().text();
    }

    if (query.numRowsAffected() > 0) emit tempestPrefixDataChanged();
}

void ParseReplyTask::upsertSuffix(QString suffix, QString description)
{
    QSqlQuery query(*p_tempest_affix_db);
    query.prepare("INSERT OR IGNORE INTO Suffix (Name, Value, Description) "
                  "VALUES (:Name,(SELECT Value FROM Suffix WHERE Name=:Name),:Description);");
    query.bindValue(":Name", suffix);
    query.bindValue(":Description", description);

    if (!query.exec())
    {
        qDebug() << "SQL query failed ( "+query.executedQuery()+"): "+query.lastError().text();
    }

    if (query.numRowsAffected() > 0) emit tempestSuffixDataChanged();
}

