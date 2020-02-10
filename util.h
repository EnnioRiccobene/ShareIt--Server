#ifndef UTIL_H
#define UTIL_H

#include <QString>

class Util
{
private:
    static QString hashString(QString string);
public:
    static QString hashStrings(QString password, QString salt);
    static QString createURI(QString filename);
    static QString checkURI(QString uri);
    static QString generateSalt();
    static void writeFile(QString filename, QByteArray fileContent);
    static QString readfile(QString filename);
    // questa l'ho creata solo per congruenza con il client
    static void showErrorAndClose(QString error);

    static bool isCompletePacket(QByteArray packet);
    // le funzioni sotto sono in comune tra server e client
    static QByteArray serialize(QStringList qStringList);
    static QList<QStringList> deserializeList(QByteArray qByteArray);

};

#endif // UTIL_H
