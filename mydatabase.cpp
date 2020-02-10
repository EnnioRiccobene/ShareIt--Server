#include "mydatabase.h"

#ifdef QT_DEBUG
const QString DB_PATH = QDir::current().path().append("/Db/db.sqlite");
#else
const QString DB_PATH = "./Db/db.sqlite";
#endif

const QString DB_TYPE = "QSQLITE";
const QString FIELD_SEPARATOR = "\r\n";

QStringList MyDatabase::login(QString emailOrUsername, QString password) {

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if (db.open()) { //Se il DB non esiste, lo crea

            QSqlDatabase::database().transaction();
            QSqlQuery query(db);

            query.prepare("SELECT rowid, username, password, salt FROM user WHERE email=? or username=?");
            query.addBindValue(emailOrUsername);
            query.addBindValue(emailOrUsername);

            if (!query.exec()) {
                qDebug() << "Errore nella query login: " + query.lastError().text();
            } else if (query.next()) {

                query.last();
                int entryNumber = query.at() + 1;

                if (entryNumber == 1) {
                    QString userId = query.value(0).toString();
                    QString username = query.value(1).toString();
                    QString hashedPassword = query.value(2).toString();
                    QString salt = query.value(3).toString();

                    if (Util::hashStrings(password, salt) != hashedPassword) {
                        response += "Wrong username/password"; // password non corrisponde
                    } else {
                        response = QStringList({"OK", userId, username});
                    }
                } else if (entryNumber >= 1) {
                    qDebug() << "ERRORE!!! ci sono piu utenti con lo stesso nome";
                } else {
                    qDebug() << "Un errore nel login";
                }
            } else {
                response += "Wrong username/password"; // emailOrUsername non esiste
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::signup(QString email, QString username, QString password) {
    /*
     * Il server si connette al db, fa, in una transazione una query count nella tabella utenti delle righe per cui username è uguale
     * al campo campo username:
     * - se il risultato è 0 faccio una query count delle email uguali all'email ricevuta:
     * -- se il risultato è zero, tramite passwordHandler genero un salt, faccio l'hash della password ricevuta, genero un colore
     * (scrivere un metodo per generarlo in maniera univoca - probabile query al db per vedere se ce ne sono uguali) e faccio una insert
     * di una riga nella tabella utenti. Poi chiudo la connection al db e chiamo il metodo createRegistrationAnswerPkt(true) e faccio una
     * write sulla socket.
     * -- se il risultato è diverso da zero, chiudo la connessione al db e chiamo il metodo createRegistrationAnswerPkt(false) e faccio
     * una write sulla socket.
     * - se il risultato è diverso da zero, chiudo la connessione al db e chiamo il metodo createRegistrationAnswerPkt(false) e faccio una
     * write sulla socket.
     */

    QStringList response = {"ERROR"}; // di default setto un errore

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) { //Se il DB non esiste, lo crea

            QSqlDatabase::database().transaction();
            QSqlQuery query(db);

            query.prepare("SELECT COUNT(*) FROM user WHERE username=?");
            query.addBindValue(username);

            if (!query.exec()) {
                qDebug() << "Errore 1 nella query signup: " + query.lastError().text();
            }
            if (query.next()) {
                int entryUsernameNumber = query.value(0).toInt();
                if(entryUsernameNumber != 0) {
                    response += "Username " + username + " already exists";
                } else {

                    query.prepare("SELECT COUNT(*) FROM user WHERE email=?");
                    query.addBindValue(email);

                    if (!query.exec()) {
                        qDebug() <<  "Errore 2 nella query signup" + query.lastError().text();
                    } else if (query.next()) {

                        int entryEmailNumber = query.value(0).toInt();
                        if(entryEmailNumber != 0) {
                            response += "Email " + email + " already exists";
                        } else {
                            QString salt = Util::generateSalt();
                            QString hashedPswd = Util::hashStrings(password, salt);

                            query.prepare("INSERT into user (email, username, password, salt) "
                                          "VALUES (?,?,?,?)");

                            query.addBindValue(email);
                            query.addBindValue(username);
                            query.addBindValue(hashedPswd);
                            query.addBindValue(salt);

                            if (!query.exec()) {
                                qDebug() <<  "Errore 3 nella query signup" + query.lastError().text();
                            } else {
                                QString userId = query.lastInsertId().toString();
                                response = QStringList({"OK", userId, username});
                            }
                        }
                    } else {
                        qDebug() <<  "Errore 4 nella query signup" + query.lastError().text();
                    }
                }
            }

            QSqlDatabase::database().commit();

            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::newfile(QString userId, QString filename) {

    QStringList response = {"ERROR"}; // di default setto un errore

    filename += " " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {

            QSqlDatabase::database().transaction();
            QSqlQuery query(db);

            QDateTime current = QDateTime::currentDateTime();
            query.prepare("INSERT into document (name) VALUES (?)");
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query newfile" + query.lastError().text();
            } else {
                QString documentId = query.lastInsertId().toString();
                query.prepare("INSERT into user_document (document_rowid, user_rowid) VALUES (?,?)");
                query.addBindValue(documentId);
                query.addBindValue(userId);

                if (!query.exec()) {
                    qDebug() <<  "Errore 2 nella query newfile" + query.lastError().text();
                } else {
                    response = QStringList({"OK", filename});
                }
            }

            QSqlDatabase::database().commit();

            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::filelist(QString userId) {

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select doc.name from user_document as ud, document as doc "
                          "where doc.rowid = ud.document_rowid and ud.user_rowid = ?");
            query.addBindValue(userId);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query filelist" + query.lastError().text();
            } else {
                response.clear();
                while (query.next()) {
                     QString filename = query.value(0).toString();
                     response += filename;
                }
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::allFilelist() {

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select name from document");

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query allFilelist" + query.lastError().text();
            } else {
                response.clear();
                while (query.next()) {
                     QString filename = query.value(0).toString();
                     response += filename;
                }
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::getSiteCounter(QString userId, QString filename) {

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select site_counter from user_document as ud, document as doc "
                          "where doc.rowid = ud.document_rowid and ud.user_rowid = ? and doc.name = ?");
            query.addBindValue(userId);
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query getFileInfo" + query.lastError().text();
            } else if (query.next()) {
                 QString siteCounter = query.value(0).toString();
                 response = QStringList{siteCounter};
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

void MyDatabase::setSiteCounter(QString userId, QString filename, ulong siteCounter) {

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("UPDATE user_document SET site_counter = ? "
                          "WHERE user_rowid = ? and document_rowid IN (SELECT rowid FROM document WHERE name = ?)");
            query.addBindValue(QString::number(siteCounter));
            query.addBindValue(userId);
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query setSiteCounter" + query.lastError().text();
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

}

QStringList MyDatabase::changeUsername(QString userId, QString newUsername) {

    /*
    * Il server si connette al db, fa, in una transazione una query count sulla tabella Utenti contanto le righe per cui username è uguale
    * a newUsername, se count è uguale a zero allora faccio l'update su username e chiudo la connessione al db, lancio
    * createChangeUsernamePasswordPkt(true) e faccio write sulla socket, altrimenti lancio createChangeUsernamePasswordPkt(false) e faccio
    * write sulla socket.
    */

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            QSqlDatabase::database().transaction();

            query.prepare("SELECT COUNT(*) FROM user WHERE username=?");
            query.addBindValue(newUsername);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query changeUsername" + query.lastError().text();
            }
            if (query.next()) {
                int entryUsernameNumber = query.value(0).toInt();
                if(entryUsernameNumber != 0) {
                    response += "Username " + newUsername + " already exists";
                } else {
                    query.prepare("UPDATE user SET username=? WHERE rowid=?");
                    query.addBindValue(newUsername);
                    query.addBindValue(userId);
                    if (!query.exec()) {
                        qDebug() <<  "Errore 2 nella query changeUsername" + query.lastError().text();
                    } else {
                        response = QStringList({"OK"});
                    }
                }
            }
            QSqlDatabase::database().commit();
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::changePassword(QString userId, QString newPassword) {

    /*
    * Il server genera hash e salt della new password, poi si connette al db, fa, in una transazione una query di update aggiornando
    * la password quando username è uguale a nickname allora chiudo la connessione al db, lancio createChangeUsernamePasswordAnswerPkt(true)
    * e faccio write sulla socket, altrimenti lancio createChangeUsernamePasswordAnswerPkt(false) e faccio write sulla socket.
    */

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            QSqlDatabase::database().transaction();

            QString salt = Util::generateSalt();
            QString hashedPassword = Util::hashStrings(newPassword, salt);

            query.prepare("UPDATE user SET password=?, salt=? WHERE rowid=?");
            query.addBindValue(hashedPassword);
            query.addBindValue(salt);
            query.addBindValue(userId);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query changePassword" + query.lastError().text();
            } else {
                response = QStringList({"OK"});
            }

            QSqlDatabase::database().commit();
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QList<QPair<QString, QString>> MyDatabase::userlist(QString filename) {

    QList<QPair<QString, QString>> response = {{"ERROR", ""}}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select ud.rowid, u.rowid, u.username from user u, document d, user_document ud "
                          "where ud.document_rowid=d.rowid and ud.user_rowid=u.rowid and d.name=? "
                          "order by ud.rowid");
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query userlist" + query.lastError().text();
            } else {
                response.clear();
                while (query.next()) {
                    QString userId = query.value(1).toString();
                    QString username = query.value(2).toString();
                    response.append({userId, username});
                }
            }
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}
QStringList MyDatabase::addUserToFile(QString filename, QString userId) {
    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("insert into user_document (document_rowid, user_rowid) "
                          "select d.rowid, u.rowid from user u, document d where u.rowid=? and d.name=?");
            query.addBindValue(userId);
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query addUserToFile" + query.lastError().text();
            } else {
                response = QStringList({"OK", filename});
            }

            QSqlDatabase::database().commit();
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

QStringList MyDatabase::addUser(QString filename, QString username) {

    QStringList response = {"ERROR"}; // di default setto un errore
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select count(*) from user where username=?");
            query.addBindValue(username);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query addUser" + query.lastError().text();
            }
            if (query.next()) {
                int entryUsernameNumber = query.value(0).toInt();
                if(entryUsernameNumber == 0) {
                    response += "This user does not exist";
                } else {
                    query.prepare("insert into user_document (document_rowid, user_rowid) "
                                  "select d.rowid, u.rowid from user u, document d where u.username=? and d.name=?");
                    query.addBindValue(username);
                    query.addBindValue(filename);

                    if (!query.exec()) {
                        qDebug() <<  "Errore 2 nella query addUser" + query.lastError().text();
                    } else {
                        response = QStringList({"OK"});
                    }
                }
            }

            QSqlDatabase::database().commit();
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}

bool MyDatabase::isAuthorized(QString userId, QString filename) {

    bool response = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE); //driver che funziona
        db.setDatabaseName(DB_PATH);

        if(db.open()) {
            QSqlQuery query(db);

            query.prepare("select count(*) from user_document ud, document d where ud.document_rowid=d.rowid and ud.user_rowid=? and d.name=?");
            query.addBindValue(userId);
            query.addBindValue(filename);

            if (!query.exec()) {
                qDebug() <<  "Errore 1 nella query isAuthorized" + query.lastError().text();
            }
            if (query.next()) {
                int entryUsernameNumber = query.value(0).toInt();
                if(entryUsernameNumber == 0) {
                    response = false;
                    qDebug() << "L'utente " + userId + " chiede l'accesso al file " + filename + " ma non è autorizzato";
                } else {
                    response = true;
                }
            }

            QSqlDatabase::database().commit();
            db.close();
        } else {
            qDebug() << "Non riesco a connettermi al DB";
        }
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    return response;
}
