#include "util.h"
#include <QCryptographicHash>
#include <QtSql>

const QString FIELD_SEPARATOR = "\r\n";

#ifdef QT_DEBUG
const QString FILES_PATH = QDir::current().path().append("/Files/");
#else
const QString FILES_PATH = "./Files/";
#endif


const QString SECRET_KEY = "la_nostra_chiave_segretissima";
const bool isDebug = false;

void Util::showErrorAndClose(QString error) {
    qDebug() << "Errore nel crdt" << error;
}

QString Util::hashStrings(QString password, QString salt) {
    if (isDebug) {
        // se sono in modalità debug non faccio hash, ritorno la password in chiaro
        return password;
    }
    QString stringToHash = password += salt;
    return hashString(stringToHash);
}
QString Util::hashString(QString string) {
    QCryptographicHash hash(QCryptographicHash::Sha3_256);
    return QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Sha3_256).toHex();
}
QString Util::createURI(QString filename) {
    return "shareit/" + filename + "/" + hashString(filename + SECRET_KEY).mid(0, 10);
}
QString Util::checkURI(QString uri) {
    QString filename = uri.split("/")[1];
    QString hash = uri.split("/")[2];
    if (hashString(filename + SECRET_KEY).mid(0, 10) == hash) {
        // se va tutto bene ritorna il nome del file
        return filename;
    } else {
        // se l'hash non corrisponde return nullptr
        return nullptr;
    }
}

void Util::writeFile(QString filename, QByteArray fileContent) {
    QFile file(FILES_PATH + filename);
    file.open(QFile::WriteOnly);
    file.write(fileContent);
}

QString Util::readfile(QString filename) {
    QFile file(FILES_PATH + filename);
    file.open(QFile::ReadOnly);
    QByteArray data = file.readAll();
    return QString::fromUtf8(data);
}

QString Util::generateSalt() {
    srand(static_cast<unsigned int>(time(nullptr)));
    size_t length = 8;
    auto randchar = []() -> char
    {
        const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    QString str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

bool Util::isCompletePacket(QByteArray packet) {
    QString packetStr = packet;
    if (packetStr.endsWith(FIELD_SEPARATOR + FIELD_SEPARATOR + FIELD_SEPARATOR + FIELD_SEPARATOR)) {
        return true;
    } else {
        return false;
    }
}

// le funzioni sotto sono "in comune" tra server e client
QByteArray Util::serialize(QStringList qStringList) {
    QString message = "";
    for(QString elem : qStringList) {
        message += elem + FIELD_SEPARATOR;
    }
    message += FIELD_SEPARATOR + FIELD_SEPARATOR + FIELD_SEPARATOR;
    return message.toUtf8();
}
QList<QStringList> Util::deserializeList(QByteArray qByteArray) {
    QString message = QString(qByteArray);
    QStringList listOfLists = message.split(FIELD_SEPARATOR + FIELD_SEPARATOR + FIELD_SEPARATOR + FIELD_SEPARATOR);
    QList<QStringList> ret;
    for (int i=0; i<listOfLists.size()-1; i++) {
        QStringList qStringList = listOfLists[i].split(FIELD_SEPARATOR);
        ret.append(qStringList);
    }
    // se il messaggio viene ricevuto correttamente finalPart è una stringa vuota,
    // altrimenti è ciò che rimane dopo l'ultimo delimitatore
    QString finalPart = listOfLists[listOfLists.size()-1];
    ret.append({finalPart});

    return ret;
}
