#ifndef SOCKETDATA_H
#define SOCKETDATA_H
#include <QString>
#include <QTcpSocket>
#include "crdtprocessor.h"
#include "mydatabase.h"
#include "myserver.h"

class MyServer; // forward declaration

class SocketData : public QObject {
    Q_OBJECT
public:
    explicit SocketData(QObject *parent = nullptr, qintptr socketDescriptor = 0, MyServer *server = nullptr);

    MyServer *server;
    QStringList readAuthenticationPacket(QStringList tokenList);
    QStringList readOtherPacket(QStringList tokenList);

    QTcpSocket *socket;
    qintptr socketDescriptor;
    QString userId;
    QString currentFilename;
    int cursorPosition;

private:
    QList<SocketData*> getAllSocketDataOfFile(QString filename);
    static void crdtProcessRemote(CrdtProcessor *crdt, QString message);

    QString messageExcess;

public slots:
    void readyRead();
    void disconnected();

};

#endif // SOCKETDATA_H
