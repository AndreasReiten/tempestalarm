#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QNetworkReply>
#include <QTime>
#include <QThreadPool>
#include <QSqlError>
#include <QNetworkRequest>
#include <QDebug>
#include <QPalette>

#include "sqlqol.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initialize();

    setDarkTheme();
}

MainWindow::~MainWindow()
{
    QThreadPool::globalInstance()->clear();

    settings.setValue("main_window/geometry", saveGeometry());
    settings.setValue("low_map_level", ui->lowMapSpinBox->value());
    settings.setValue("high_map_level", ui->highMapSpinBox->value());
    settings.setValue("affix_value_alarm_threshold", ui->alarmThresholdSpinBox->value());
    settings.setValue("alarm_confirmation_time", ui->alarmWaitSpinBox->value());
    settings.setValue("alarm_volume", ui->volumeSlider->value());
    settings.setValue("min_votes", ui->minVoteSpinBox->value());

//    if (map_db.isOpen()) map_db.close();
//    if (tempest_affix_db.isOpen()) tempest_affix_db.close();
    delete ui;
}

void MainWindow::setDarkTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15,15,15));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");


    this->setPalette(palette);
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(palette);
}

void MainWindow::initDataBases()
{
    // Map database
    if (map_db.isOpen()) map_db.close();
    map_db = QSqlDatabase::addDatabase("QSQLITE", "MapDB");

    map_db.setDatabaseName(QDir::currentPath() + "/map_data.sqlite3");

    if (map_db.open())
    {
        QSqlQuery query(map_db);
        if (!query.exec("CREATE TABLE IF NOT EXISTS Maps ("
                        "Name TEXT PRIMARY KEY NOT NULL, "
                        "Level INT, "
                        "TempestPrefix TEXT, "
                        "TempestSuffix TEXT, "
                        "TempestValue INT, "
                        "Votes INT);"))
        {
            qDebug() << sqlQueryError(query);
        }
    }
    else
    {
        qDebug() << "Database error" << map_db.lastError();
    }

    // Tempest affix database
    if (tempest_affix_db.isOpen()) tempest_affix_db.close();
    tempest_affix_db = QSqlDatabase::addDatabase("QSQLITE", "TempestAffixDB");

    tempest_affix_db.setDatabaseName(QDir::currentPath() + "/tempest_affix_data.sqlite3");

    if (tempest_affix_db.open())
    {
        QSqlQuery query(tempest_affix_db);
        if (!query.exec("CREATE TABLE IF NOT EXISTS Prefix ("
                        "Name TEXT PRIMARY KEY NOT NULL, "
                        "Value INT,"
                        "Description TEXT);"))
        {
            qDebug() << sqlQueryError(query);
        }

        if (!query.exec("CREATE TABLE IF NOT EXISTS Suffix ("
                        "Name TEXT PRIMARY KEY NOT NULL, "
                        "Value INT,"
                        "Description TEXT);"))
        {
            qDebug() << sqlQueryError(query);
        }
    }
    else
    {
        qDebug() << "Database error" << map_db.lastError();
    }
}

void MainWindow::setUpdateInterval(int value)
{
    update_timer->start(value*1000);
}

void MainWindow::initialize()
{
    // Load settings
    this->restoreGeometry(settings.value("main_window/geometry").toByteArray());
    ui->lowMapSpinBox->setValue(settings.value("low_map_level",68).toInt());
    ui->highMapSpinBox->setValue(settings.value("high_map_level",84).toInt());
    ui->alarmThresholdSpinBox->setValue(settings.value("affix_value_alarm_threshold",50).toInt());
    ui->alarmWaitSpinBox->setValue(settings.value("alarm_confirmation_time",180).toInt());
    ui->volumeSlider->setValue(settings.value("alarm_volume",50).toInt());
    ui->minVoteSpinBox->setValue(settings.value("min_votes",3).toInt());

    // Set alarm audio file
    alarm_effect.setSource(QUrl::fromLocalFile("sound/codeccall.wav"));
    alarm_effect.setLoopCount(QSoundEffect::Infinite);
    alarm_effect.setVolume(1.0f);
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));

    // Init databases
    initDataBases();

    // Init web view with poetempest.com
    web_engine = new QWebEngineView;
    web_engine->load(QUrl("http://poetempest.com/"));
    ui->tabWidget->insertTab(2, web_engine, "Tempest Watch");
    ui->tabWidget->setCurrentIndex(0);
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    // Init model/view for SQLITE table "Maps"
    map_query = "SELECT * FROM Maps ORDER BY TempestValue DESC";

    map_model = new CustomSqlQueryModel(ui->mapTableView);
    map_model->setQuery(map_query, map_db);
    map_model->setHeaderData(0, Qt::Horizontal, tr("Map"));
    map_model->setHeaderData(1, Qt::Horizontal, tr("Level"));
    map_model->setHeaderData(2, Qt::Horizontal, tr("Prefix"));
    map_model->setHeaderData(3, Qt::Horizontal, tr("Suffix"));
    map_model->setHeaderData(4, Qt::Horizontal, tr("Affix value"));
    map_model->setHeaderData(5, Qt::Horizontal, tr("Votes"));

    map_header = ui->mapTableView->horizontalHeader();
    map_header->setSortIndicatorShown(true);
    map_header->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->mapTableView->setModel(map_model);
    ui->mapTableView->verticalHeader()->setVisible(false);
    connect(map_header, SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setMapQuery(int,Qt::SortOrder)));

    // Init model/view for SQLITE table "Prefix"
    tempest_prefix_model = new RestrictiveSqlTableModel(ui->tempestPrefixTableView, tempest_affix_db);
    tempest_prefix_model->setTable("Prefix");
    tempest_prefix_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    tempest_prefix_model->setSort(1,Qt::DescendingOrder);
    tempest_prefix_model->select();
    tempest_prefix_model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    tempest_prefix_model->setHeaderData(1, Qt::Horizontal, tr("Value"));
    tempest_prefix_model->setHeaderData(2, Qt::Horizontal, tr("Description"));

    tempest_prefix_header = ui->tempestPrefixTableView->horizontalHeader();
    tempest_prefix_header->setSortIndicatorShown(true);
    tempest_prefix_header->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(tempest_prefix_header, SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setPrefixTableSort(int,Qt::SortOrder)));

    ui->tempestPrefixTableView->setModel(tempest_prefix_model);
    ui->tempestPrefixTableView->verticalHeader()->setVisible(false);

    // Init model/view for SQLITE table "Suffix"
    tempest_suffix_model = new RestrictiveSqlTableModel(ui->tempestSuffixTableView, tempest_affix_db);
    tempest_suffix_model->setTable("Suffix");
    tempest_suffix_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    tempest_suffix_model->setSort(1,Qt::DescendingOrder);
    tempest_suffix_model->select();
    tempest_suffix_model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    tempest_suffix_model->setHeaderData(1, Qt::Horizontal, tr("Value"));
    tempest_suffix_model->setHeaderData(2, Qt::Horizontal, tr("Description"));

    tempest_suffix_header = ui->tempestSuffixTableView->horizontalHeader();
    tempest_suffix_header->setSortIndicatorShown(true);
    tempest_suffix_header->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->tempestSuffixTableView->setModel(tempest_suffix_model);
    ui->tempestSuffixTableView->verticalHeader()->setVisible(false);

    connect(tempest_suffix_header, SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setSuffixTableSort(int,Qt::SortOrder)));

    connect(ui->mapTableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(mapTableDoubleClicked(QModelIndex)));

    // Init network
    network_manager = new QNetworkAccessManager;
    connect(network_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReplyFinished(QNetworkReply*)));

    // Init repeating updates
    update_timer = new QTimer;
    connect(update_timer, SIGNAL(timeout()), this, SLOT(pullMaps()));
    update_timer->start(ui->updateSpinBox->value()*1000);

    connect(ui->updateSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateInterval(int)));

    // Init alarm timer. Starts if a hot map is detected. If a hot map is still detected after the timeout
    alarm_confirmation_timer = new QTimer;
    alarm_confirmation_timer->setSingleShot(true);
    connect(alarm_confirmation_timer, SIGNAL(timeout()), this, SLOT(playAlarm()));


    // Snooze timer
    snooze_timer = new QTimer;
    snooze_timer->setSingleShot(true);
    connect(ui->snoozePushButton, SIGNAL(pressed()), this, SLOT(snoozeAlarm()));
    connect(ui->unSnoozePushButton, SIGNAL(pressed()),this,SLOT(unSnoozeAlarm()));
    connect(snooze_timer, SIGNAL(timeout()),this,SLOT(unSnoozeAlarm()));

    // Init active map range
    low_map_level = ui->lowMapSpinBox->value();
    high_map_level = ui->highMapSpinBox->value();
    connect(ui->lowMapSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setLowMapLevel(int)));
    connect(ui->highMapSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setHighMapLevel(int)));

    // Init alarm timer threshold
    alarm_value_threshold = ui->alarmThresholdSpinBox->value();
    alarm_wait_time = ui->alarmWaitSpinBox->value();
    connect(ui->alarmThresholdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAlarmValueThreshold(int)));
    connect(ui->alarmWaitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAlarmConfirmationTime(int)));

    // Init map resolve timer
    resolve_maps_timer = new QTimer;
    connect(resolve_maps_timer, SIGNAL(timeout()), this, SLOT(resolveMapTempests()));
    resolve_maps_timer->start(500);

    // Init min vote spin box
    connect(ui->minVoteSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setMinVotes(int)));

    pullMaps();
}

void MainWindow::tabChanged(int index)
{
    if (index == 2)
    {
        web_engine->reload();
    }
}

void MainWindow::setVolume(int value)
{
    alarm_effect.setVolume(((double)value)*0.01);
}

void MainWindow::setPrefixTableSort(int column, Qt::SortOrder order)
{
    tempest_prefix_model->sort(column,order);
}

void MainWindow::setSuffixTableSort(int column, Qt::SortOrder order)
{
    tempest_suffix_model->sort(column,order);
}

void MainWindow::mapTableDoubleClicked(const QModelIndex & index)
{
    if (index.column() == 2)
    {
        // Do something on double click
//        index.data().toString();
    }
}

void MainWindow::setMinVotes(int value)
{
    min_votes = value;
}

void MainWindow::setLowMapLevel(int value)
{
    low_map_level = value;
}

void MainWindow::setHighMapLevel(int value)
{
    high_map_level = value;
}

void MainWindow::setAlarmValueThreshold(int value)
{
    alarm_value_threshold = value;
}

void MainWindow::setAlarmConfirmationTime(int value)
{
    alarm_wait_time = value;
    if (alarm_confirmation_timer->isActive())
    {
        alarm_confirmation_timer->start(value*1000);
    }
}

void MainWindow::playAlarm()
{
    alarm_effect.play();
    ui->snoozePushButton->setEnabled(true);
}

void MainWindow::snoozeAlarm()
{
    alarm_confirmation_timer->stop();
    alarm_effect.stop();

    int time_until_shift = 60 - QTime::currentTime().minute();

    snooze_timer->start(time_until_shift*60*1000);

    ui->infoLabel->setText("Snoozed!");
    ui->snoozePushButton->setEnabled(false);
    ui->unSnoozePushButton->setEnabled(true);
}

void MainWindow::unSnoozeAlarm()
{
    snooze_timer->stop();

    ui->infoLabel->setText("Unsnoozed...");

    ui->unSnoozePushButton->setEnabled(false);
}

void MainWindow::resolveMapTempests()
{
    if (snooze_timer->isActive())
    {
        ui->infoLabel->setText("Snoozed until Tempest shift in "+QString::number(snooze_timer->remainingTime()/1000/60)+" min");
        return;
    }

    QSqlQuery query(map_db);
    query.prepare("SELECT * FROM Maps WHERE TempestValue >=:Threshold AND Level >= :LowMapLevel AND Level <= :HighMapLevel AND Votes >= :MinVotes;");
    query.bindValue(":Threshold", alarm_value_threshold);
    query.bindValue(":LowMapLevel", low_map_level);
    query.bindValue(":HighMapLevel", high_map_level);
    query.bindValue(":MinVotes", min_votes);
    if (!query.exec())
    {
        qDebug() << sqlQueryError(query);
    }

    if (query.next())
    {
        if (alarm_confirmation_timer->isActive())
        {
            ui->infoLabel->setText("One or more maps ("+QString::number(low_map_level)+"-"+QString::number(high_map_level)+") exceed the  affix value alarm threshold! Sounding alarm in "+QString::number(alarm_confirmation_timer->remainingTime()/1000)+" sec");
        }
        else
        {
            if (!alarm_effect.isPlaying()) alarm_confirmation_timer->start(alarm_wait_time*1000);
            ui->infoLabel->setText("One or more maps ("+QString::number(low_map_level)+"-"+QString::number(high_map_level)+") exceed the  affix value alarm threshold!");
        }

    }
    else
    {
        alarm_confirmation_timer->stop();
        alarm_effect.stop();
        ui->snoozePushButton->setEnabled(false);
        ui->infoLabel->setText("No maps ("+QString::number(low_map_level)+"-"+QString::number(high_map_level)+") exceed the affix value alarm threshold.");
    }
}

void MainWindow::setMapQuery(int section, Qt::SortOrder order)
{
    QMap<Qt::SortOrder,QString> order_map;
    order_map[Qt::AscendingOrder] = "ASC";
    order_map[Qt::DescendingOrder] = "DESC";

    QMap<QString,QString> section_map;
    section_map["Map"] = "Name";
    section_map["Level"] = "Level";
    section_map["Prefix"] = "TempestPrefix";
    section_map["Suffix"] = "TempestSuffix";
    section_map["Affix value"] = "TempestValue";
    section_map["Votes"] = "Votes";

    map_query = ("SELECT * FROM Maps ORDER BY "+
                        section_map[map_model->headerData(section, Qt::Horizontal).toString()]+
                        " "+order_map[order]);

    map_model->setQuery(map_query, map_db);
}

void MainWindow::pullMaps()
{
    QNetworkRequest request(QUrl("http://poetempest.com/"));
    request.setRawHeader("Accept-Encoding", "gzip,deflate");
    network_manager->get(request);
}

void MainWindow::onReplyFinished(QNetworkReply * reply)
{
    // Check for HTML error
    if (reply->error() != 0)
    {
        qDebug() << "Network request status code "+QString::number(int(reply->error()))+" ("+reply->url().toString()+"): "+reply->errorString();
    }

    ParseReplyTask *task = new ParseReplyTask(map_db, tempest_affix_db, reply);
    task->setAutoDelete(true);

    connect(task, SIGNAL(mapDataChanged()), this, SLOT(updateMapTable()));
    connect(task, SIGNAL(tempestPrefixDataChanged()), this, SLOT(updatePrefixTable()));
    connect(task, SIGNAL(tempestSuffixDataChanged()), this, SLOT(updateSuffixTable()));
    QThreadPool::globalInstance()->start(task);
}

void MainWindow::updateMapTable()
{
    map_model->setQuery(map_query, map_db);
    if (map_model->lastError().isValid())
    {
        qDebug() << map_model->lastError();
    }
}

void MainWindow::updatePrefixTable()
{
    tempest_prefix_model->select();
}

void MainWindow::updateSuffixTable()
{
    tempest_suffix_model->select();
}

void MainWindow::dumpString(QString str)
{
    QFile file("stringdump.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << str;

    file.close();
}
