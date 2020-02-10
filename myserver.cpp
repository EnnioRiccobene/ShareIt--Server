// myserver.cpp

#include <qtconcurrentrun.h>

#include "myserver.h"
#include "socketdata.h"

MyServer::MyServer(QObject *parent) : QTcpServer(parent) {}

void MyServer::startServer() {
    quint16 port = 1234;

    if(!this->listen(QHostAddress::Any, port)) {
        qDebug() << "Could not start server";
    } else {
        qDebug() << "Listening to port " << port << "...";
    }

    readAllFiles();

}

void MyServer::incomingConnection(qintptr socketDescriptor) {
    // We have a new connection
    qDebug() << socketDescriptor << " Connecting...";

    SocketData *socketData = new SocketData(nullptr, socketDescriptor, this);
    sockets.insert(socketDescriptor, socketData);
    qDebug() << "Ci sono" << sockets.count() << "socket aperti";
}

void MyServer::readAllFiles() {

    QStringList fileNames = MyDatabase::allFilelist();

    for (QString filename : fileNames) {
        QString fileContent = Util::readfile(filename);

        CrdtProcessor *crdt = new CrdtProcessor(this, fileContent, 0, 0);
        this->files.insert(filename, crdt);
    }

    QtConcurrent::run(MyServer::saveFiles, this->files);

}

void MyServer::saveFiles(QHash<QString, CrdtProcessor*> files) {
    while (true) {
        for (QString filename : files.keys()) {
            CrdtProcessor *crdt = files.find(filename).value();
            QtConcurrent::run(Util::writeFile, filename, crdt->serializeContent().toUtf8());
            QThread::sleep(10); // ogni 10 secondi riscrive i file su disco
        }
    }
}
