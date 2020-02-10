#include <QPair>

#include <qtconcurrentrun.h>
#include "socketdata.h"

SocketData::SocketData(QObject *parent, qintptr socketDescriptor, MyServer *server) : QObject(parent) {
    this->server = server;
    this->socketDescriptor = socketDescriptor;
    socket = new QTcpSocket();
    if(!socket->setSocketDescriptor(this->socketDescriptor)) {
        qDebug() << "C'è un problema nella creazione del socket...";
        return;
    }
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    messageExcess = "";
    qDebug() << socketDescriptor << " Client connected";
}

void SocketData::disconnected() {
    qDebug() << socketDescriptor << "Disconnected";
    // simulo che il client mi abbia mandato una opcursorpos con -1, ovvero ha chiuso il file (se ne ha uno aperto)
    this->readOtherPacket({"opcursorpos", "-1"});

    server->sockets.remove(this->socketDescriptor);
    socket->deleteLater();
}

void SocketData::readyRead() {

    QByteArray request = socket->readAll();
    // devo trattare il pacchetto che mi arriva come una lista di pacchetti
    // perchè a volte ne arrivano due contemporaneamente (es: opsendupdate con opcursorpos)
    request = messageExcess.toUtf8() + request;
    QList<QStringList> tokenLists = Util::deserializeList(request);

    messageExcess = tokenLists[tokenLists.size()-1][0];
    tokenLists.removeLast();
    // TODO da rimuovere
//    if (messageExcess != "") {
//        qDebug() << "messageExcess :" << messageExcess;
//    }

    if (tokenLists[0][0] == "opsendupdate") {
        while (true) {
            if (Util::isCompletePacket(request)) { // se il pacchetto è completo esco
                break;
            }
//            qDebug() << "Mi manca un pezzo del pacchetto";
            if (socket->waitForReadyRead(3000)) { // per 3 secondi attendo il resto del pacchetto
                QByteArray continuedPacket = socket->readAll();
                request += continuedPacket;
            } else { // se non arriva più nulla esco
                break;
            }
        }
        tokenLists = Util::deserializeList(request);
        messageExcess = tokenLists[tokenLists.size()-1][0];
        tokenLists.removeLast();
        // TODO da rimuovere
//        if (messageExcess != "") {
//            qDebug() << "messageExcess :" << messageExcess;
//        }

    }

    for (QStringList tokenList : tokenLists) {

        if (tokenList[0] != "opsendupdate" && tokenList[0] != "opcursorpos" &&
                tokenList[0] != "opsignup" && tokenList[0] != "oplogin") {
            qDebug() << socketDescriptor << " Data in: " << tokenList;
        } else if (tokenList[0] == "opcursorpos") {
//            qDebug() << socketDescriptor << " Data in: " << tokenList;
        } else if (tokenList[0] == "oplogin") {
            qDebug() << socketDescriptor << " Data in: " << tokenList[0] << tokenList[1];
        } else if (tokenList[0] == "opsignup") {
            qDebug() << socketDescriptor << " Data in: " << tokenList[0] << tokenList[1] << tokenList[2];
        }

        /*
        else if (tokenList[0] == "opsendupdate") { // se mando un update lungo stampo solo la prima parte per non rallentare tutto
            qDebug() << socketDescriptor << " Data in: "
                     << QStringList{tokenList[0], tokenList[1], tokenList[2].mid(0, 50) + "..."};
        }
        */

        QStringList response;
        QString opcode = tokenList[0];
        if (this->userId == nullptr) {
            // l'utente deve autenticarsi, con login o signup
            response = readAuthenticationPacket(tokenList);
        } else {
            response = readOtherPacket(tokenList);
            if (opcode == "opsendupdate" || opcode == "opcursorpos" ||
                    opcode == "opclosefile") {
                // non torno nulla al client, ho mandato gli update agli altri client
                continue;
            }
        }
        response.insert(0, opcode);

        QByteArray byteArray = Util::serialize(response);

        if (opcode != "opopenfile") {
            qDebug() << socketDescriptor << " Data out: " << response;
            socket->write(byteArray);
        } else {
            int sizeOfPacket = 1000; // dimensione del pacchetto in bytes
            int numOfPackets = (byteArray.size() / sizeOfPacket) + 1;
            response[1] = QString::number(byteArray.size()).rightJustified(10, '0');
            byteArray = Util::serialize(response); // ricrea byteArray con la sua dimensione
            // se mando un file lungo stampo solo la prima parte per non rallentare tutto
            qDebug() << socketDescriptor << " Data out: "
                     << QStringList{response[0], response[1], response[2], response[3], response[4].mid(0, 40) + "..."};

            for (int i=0; i<numOfPackets; i++) { // invio più pacchetti di dimensione sizeOfPacket
                QByteArray partOfByteArray = byteArray.mid(i*sizeOfPacket, sizeOfPacket);
                socket->write(partOfByteArray);
            }
        }

    }
}


QStringList SocketData::readOtherPacket(QStringList tokenList) {

    QString opcode = tokenList[0];

    if (opcode == "oplogout") {
        this->userId = nullptr;
        return {"OK"};

    } else if (opcode == "opchangeusername") {
        QString newUsername = tokenList[1];
        QStringList response = MyDatabase::changeUsername(this->userId, newUsername);
        return response;

    } else if (opcode == "opchangepassword") {
        QString newPassword = tokenList[1];
        QStringList response = MyDatabase::changePassword(this->userId, newPassword);
        return response;

    } else if (opcode == "opnewfile") {
        QString filename = tokenList[1];
        QStringList response = MyDatabase::newfile(this->userId, filename);
        filename = response[1];

        CrdtProcessor *crdt = new CrdtProcessor(this, CRDT_PROC_SERVER_MODE, 0, 0);
        Util::writeFile(filename, crdt->serializeContent().toUtf8());
        this->server->files.insert(filename, crdt);

        return response;

    } else if (opcode == "opaddfile") {
        QString uri = tokenList[1];
        QString filename = Util::checkURI(uri);
        if (filename == nullptr) {
            return {"ERROR", "Wrong link"};
        }
        return MyDatabase::addUserToFile(filename, this->userId);

    } else if (opcode == "opfilelist") {
        QStringList filelist = MyDatabase::filelist(this->userId);
        return filelist;

    } else if (opcode == "opopenfile") {
        QString filename = tokenList[1];
        if (MyDatabase::isAuthorized(this->userId, filename)) {

            QString filecontent = this->server->files.find(filename).value()->serializeContent();

            this->currentFilename = filename;
            this->cursorPosition = 0;
            QString uri = Util::createURI(filename);
            QString siteCounter = MyDatabase::getSiteCounter(this->userId, filename)[0];

            // simulo che l'utente abbia spostato il cursore in posizione 0
            readOtherPacket({"opcursorpos", "0"});

            QString sizeOfPacket = "1234567890"; // viene settato in seguito

            return {sizeOfPacket, uri, siteCounter, filecontent};
        } else {
            return {"ERROR"}; // l'utente non è autorizzato
        }

    } else if (opcode == "opclosefile") {
        // comunico agli altri utenti che sono uscito
        this->readOtherPacket({"opcursorpos", "-1"});

        this->currentFilename = nullptr;
        this->cursorPosition = -1;
        return {};

    } else if (opcode == "opuserlist") {
        QList<QPair<QString, QString>> userList = MyDatabase::userlist(this->currentFilename);
        if (userList[0].first == "ERROR") {
            return {"ERROR"};
        }

        QHash<QString, SocketData*> mapUserId_socketData;
        for(SocketData *sd : getAllSocketDataOfFile(this->currentFilename)) {
            mapUserId_socketData.insert(sd->userId, sd);
        }
        QStringList triplet_userId_username_cursorPosition;
        for (QPair<QString, QString> pair : userList) {
            int cursorPos = -1;
            if (mapUserId_socketData.contains(pair.first)) {
                cursorPos = mapUserId_socketData.find(pair.first).value()->cursorPosition;
            }
            triplet_userId_username_cursorPosition.append(pair.first);
            triplet_userId_username_cursorPosition.append(pair.second);
            triplet_userId_username_cursorPosition.append(QString::number(cursorPos));
        }
        return triplet_userId_username_cursorPosition;

    } else if (opcode == "opadduser") {
        QString username = tokenList[1];
        return MyDatabase::addUser(this->currentFilename, username);

    } else if (opcode == "opsendupdate") {
        ulong siteCounter = tokenList[1].toULong();
        QString message = tokenList[2];
        CrdtProcessor *crdt = this->server->files.find(this->currentFilename).value();
        QFuture<void> future1 = QtConcurrent::run(SocketData::crdtProcessRemote, crdt, message);
        QFuture<void> future2 = QtConcurrent::run(MyDatabase::setSiteCounter,
                                                  this->userId, this->currentFilename, siteCounter);

        for(SocketData *sd : getAllSocketDataOfFile(this->currentFilename)) {
            if (this->socketDescriptor != sd->socketDescriptor) { // mando a tutti tranne me stesso
                sd->socket->write(Util::serialize({"opsendupdate", message.toUtf8()}));
            }
        }

        future1.waitForFinished();
        future2.waitForFinished();

        return {};

    } else if (opcode == "opcursorpos") {
        if (this->currentFilename == nullptr) {
            return {};
        }
        this->cursorPosition = tokenList[1].toInt();
        for(SocketData *sd : getAllSocketDataOfFile(this->currentFilename)) {
            if (this->socketDescriptor != sd->socketDescriptor) { // mando a tutti tranne me stesso
                sd->socket->write(Util::serialize({"opcursorpos", this->userId, QString::number(cursorPosition)}));
            }
        }
        return {};

    } else {
        qDebug() << "è stato ricevuto un opcode sconosciuto" << opcode << "nel messaggio" << tokenList[0].mid(0, 10) << "...";
        return {"ERROR"};
    }

}
void SocketData::crdtProcessRemote(CrdtProcessor *crdt, QString message) {
    crdt->processRemote(message);
}

QStringList SocketData::readAuthenticationPacket(QStringList tokenList) {

    // questa funzione viene chiamata quando l'utente non è ancora autenticato. l'opcode deve essere oplogin o opsignup

    QString opcode = tokenList[0];

    if(opcode == "oplogin") {

        QString emailOrUsername = tokenList[1];
        QString password = tokenList[2];

        QStringList response = MyDatabase::login(emailOrUsername, password);
        if (response[0] == "OK") {
            for (SocketData *sd : this->server->sockets) {
                if (sd->userId == response[1]) { // l'utente è già loggato
                    return {"ERROR", "User is already logged in"};
                }
            }
            this->userId = response[1];
        }
        return response;

    } else if (opcode == "opsignup") {

        QString email = tokenList[1];
        QString username = tokenList[2];
        QString password = tokenList[3];

        QStringList response = MyDatabase::signup(email, username, password);
        if (response[0] == "OK") {
            this->userId = response[1];
        }
        return response;

    } else {
        qDebug() << "Un utente non autenticato ha inviato il seguente messaggio:" << tokenList;
        return {"ERROR"};
    }
}

// questa funzione restituisce tutti i SocketData che hanno il file "filename" aperto
QList<SocketData*> SocketData::getAllSocketDataOfFile(QString filename) {
    QList<SocketData*> list;
    for(SocketData *sd : this->server->sockets) {
        if (filename == sd->currentFilename) {
            list.append(sd);
        }
    }
    return list;
}
