#include <QApplication>
#include <QDir>
#include <QSettings>
#include "AppWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("HabitTrackerTDD"));
    QApplication::setOrganizationName(QStringLiteral("HabitTrackerTDD"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setFont(QFont(QStringLiteral("Segoe UI"), 10));
    const auto configuration = QDir(qEnvironmentVariable("LOCALAPPDATA"))
        .filePath(QStringLiteral("HabitTrackerTDD/connection.ini"));
    QSettings settings(configuration, QSettings::IniFormat);
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    db.setHostName(settings.value(QStringLiteral("host"), QStringLiteral("localhost")).toString());
    db.setPort(settings.value(QStringLiteral("port"), 5432).toInt());
    db.setDatabaseName(settings.value(QStringLiteral("database"), QStringLiteral("habit_tracker_tdd")).toString());
    db.setUserName(settings.value(QStringLiteral("user"), QStringLiteral("habit_tracker_tdd_app")).toString());
    db.setConnectOptions(QStringLiteral("connect_timeout=3"));
    int result;
    {
        AppWindow window(db);
        window.show();
        result = app.exec();
    }
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return result;
}
