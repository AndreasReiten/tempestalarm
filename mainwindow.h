#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "restrictivesqltablemodel.h"
#include "customsqlquerymodel.h"
#include "parsereplytask.h"

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QWebEngineView>
#include <QTimer>
#include <QSoundEffect>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setMapQuery(int section, Qt::SortOrder order);
    void onReplyFinished(QNetworkReply * reply);
    void setUpdateInterval(int value);
    void updateMapTable();
    void updatePrefixTable();
    void updateSuffixTable();
    void resolveMapTempests();
    void playAlarm();
    void snoozeAlarm();
    void unSnoozeAlarm();
    void setLowMapLevel(int value);
    void setHighMapLevel(int value);
    void setAlarmValueThreshold(int value);
    void setAlarmConfirmationTime(int value);
    void pullMaps();
    void setPrefixTableSort(int column, Qt::SortOrder order);
    void setSuffixTableSort(int column, Qt::SortOrder order);
    void mapTableDoubleClicked(const QModelIndex &index);
    void tabChanged(int index);
    void setVolume(int value);
    void setMinVotes(int value);

private:
    Ui::MainWindow *ui;

    QWebEngineView * web_engine;

    QNetworkAccessManager * network_manager;

    QSqlDatabase map_db;
    QSqlDatabase tempest_affix_db;

    CustomSqlQueryModel * map_model;
    QHeaderView * map_header;
    QString map_query;

    RestrictiveSqlTableModel * tempest_prefix_model;
    QHeaderView * tempest_prefix_header;

    RestrictiveSqlTableModel * tempest_suffix_model;
    QHeaderView * tempest_suffix_header;

    void initDataBases();
    void initialize();
    void dumpString(QString str);
    void setDarkTheme();

    QTimer * snooze_timer;
    QTimer * alarm_confirmation_timer;
    QTimer * update_timer;
    QTimer * resolve_maps_timer;
    QSoundEffect alarm_effect;

    int low_map_level;
    int high_map_level;
    int alarm_value_threshold;
    int alarm_wait_time;
    int min_votes;

    QSettings settings;
};

#endif // MAINWINDOW_H
