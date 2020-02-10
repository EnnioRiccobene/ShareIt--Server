#ifndef CRDTPROCESSOR_H
#define CRDTPROCESSOR_H
#include <QObject>
#include <QMap>
#include "EditorTypes.h"
#include "symbol.h"







class CrdtProcessor : public QObject {
    Q_OBJECT

private:
    ulong siteId;
    ulong siteCnt;
    QMap<Symbol,QString> map;
    QMap<Symbol,ulong> idMap;
    QRegularExpression regex_map;
    QRegularExpression regex_operation;
    Symbol insertSymbol(int pos, QChar element);
    Symbol deleteSymbol(int pos);
static QList<Symbol> parseMap(QString textMap);
static QString serializeOp(ENUM_EDIT_CODE type, Symbol& s);

public:
     CrdtProcessor(QObject * parent, QString fileContent,
                           ulong siteId, ulong siteCnt);

    QString processLocal(QVector<ENUM_EDIT_CODE> operationType, QVector<int> positions, QVector<QChar> characters);
    QVector<ulong> processRemote(QString message);
    QString serializeContent();
    QString getCurrentContentPlainText();
    ulong getSiteCnt() const;
    QList<ulong> getSymIdList();
    QString printRemote(QString message);
};

#endif // CRDTPROCESSOR_H
