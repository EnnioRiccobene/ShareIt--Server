#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include "socketdata.h"
#include "crdtprocessor.h"

class SocketData; // forward declaration

class MyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyServer(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor);

    void readAllFiles();
    static void saveFiles(QHash<QString, CrdtProcessor*> files);

public:
    QHash<qintptr, SocketData*> sockets;
    QHash<QString, CrdtProcessor*> files;
};

#endif // MYSERVER_H
