#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QtSql>
#include <QSqlDatabase>
#include "util.h"

class MyDatabase {
public:

    static QStringList login(QString emailOrUsername, QString password);
    static QStringList signup(QString email, QString username, QString password);
    static QStringList changeUsername(QString userId, QString newUsername);
    static QStringList changePassword(QString userId, QString newPassword);
    static QStringList newfile(QString userId, QString filename);
    static QStringList filelist(QString userId);
    static QStringList allFilelist();
    static QStringList getSiteCounter(QString userId, QString filename);
    static void setSiteCounter(QString userId, QString filename, ulong siteCounter);
    static QList<QPair<QString, QString>> userlist(QString filename);
    static QStringList addUser(QString filename, QString username);
    static QStringList addUserToFile(QString filename, QString userId);
    static bool isAuthorized(QString userId, QString filename);

};

#endif // DATABASE_H
