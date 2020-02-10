#ifndef SYMBOL_H
#define SYMBOL_H

#include <QObject>
#include "EditorTypes.h"
#include "util.h"
#define SYMBOL_FIELDS_NUMBER 5
class Symbol {


    //content
    QChar element;
    //Symbol key
    ulong num;
    ulong den;
    ulong symbolSite;
    ulong symbolCnt;

    explicit Symbol();
    Symbol(QChar element, ulong num, ulong den, ulong symbolSite,
           ulong symbolCnt);

    static std::pair<ulong , ulong>
    createFractional(ENUM_INSERT_TYPE where,const Symbol& lSym,const Symbol& rSym);



public:


        static Symbol nullSymbol();

        static QString serializeSymbol(Symbol& s);


        static Symbol createSymbol(QChar c,ulong symbolSite,
                                   ulong symbolCnt, ENUM_INSERT_TYPE where, const Symbol& lSym, const Symbol& rSym);

        static Symbol parseSymbol(QStringList fields);

        QChar getElement() const;

        ulong getNum() const;

        ulong getDen() const;

        ulong getSymbolSite() const;

        ulong getSymbolCnt() const;

        bool operator<(const Symbol &rhs) const;

        bool operator==(const Symbol &rhs) const;

        bool isNull() const;

        QString toString();
};

#endif // SYMBOL_H
