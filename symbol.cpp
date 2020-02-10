
#include "symbol.h"

//null symbol constructor
Symbol::Symbol(): element(QChar(0)),num(0),den(0),symbolSite(0),symbolCnt(0){}

Symbol::Symbol(QChar element, ulong num, ulong den, ulong symbolSite, ulong symbolCnt):element(element),
    num(num),den(den),symbolSite(symbolSite),symbolCnt(symbolCnt){}

std::pair<ulong, ulong> Symbol::createFractional(ENUM_INSERT_TYPE where,const Symbol& lSym, const Symbol& rSym)
{
    ulong num, den;

    switch (where) {
        case INSERT_HEAD:
        if(rSym.isNull()){
            throw std::logic_error("Symbol Fractional index generation gone bad");
        }
            num = rSym.getNum();
            den = rSym.getDen()+1;
            break;
        case INSERT_TAIL:
        if(lSym.isNull()){
            throw std::logic_error("Symbol Fractional index generation gone bad");
        }
            num = lSym.getNum()+1;
            den = lSym.getDen();
            break;
        case INSERT_BETWEEN:
        if(lSym.isNull() || rSym.isNull()){
            throw std::logic_error("Symbol Fractional index generation gone bad");
        }
            num = lSym.getNum() + rSym.getNum();
            den = lSym.getDen() + rSym.getDen();
            break;
        case INSERT_FIRST:
        num = den= 1;
        break;
        default:
            num = den = 0;
        break;
    }
    if(!num || !den) {
        throw std::logic_error("Symbol Fractional index generation gone bad");
    }
    return std::pair<ulong, ulong>(num, den);
}

Symbol Symbol::nullSymbol(){
    return Symbol();
}

QString Symbol::serializeSymbol(Symbol &s)
{
    QStringList text;
    text.append(QString(s.element));
    text.append(QString::number(s.num));
    text.append(QString::number(s.den));
    text.append(QString::number(s.symbolSite));
    text.append(QString::number(s.symbolCnt));
    QString res = text.join(CRDT_SYMBOL_FIELD_TAG) % CRDT_SYMBOL_FIELD_TAG % CRDT_RECORD_TAG;
    return res;

}

Symbol Symbol::createSymbol(QChar c, ulong symbolSite, ulong symbolCnt, ENUM_INSERT_TYPE where,const Symbol& lSym,const Symbol& rSym){
    std::pair<ulong, ulong> fract = Symbol::createFractional(where,lSym,rSym);
    Symbol s(c,fract.first,fract.second,symbolSite,symbolCnt);
    return s;

}

Symbol Symbol::parseSymbol(QStringList fields){
    QChar c = fields.at(0).front();
    ulong num = fields.at(1).toULong();
    ulong den = fields.at(2).toULong();
    ulong symSite = fields.at(3).toULong();
    ulong symCnt = fields.at(4).toULong();

    return Symbol(c,num,den,symSite,symCnt);
}

QChar Symbol::getElement() const {
    return element;
}

ulong Symbol::getNum() const {
    return num;
}

ulong Symbol::getDen() const {
    return den;
}

ulong Symbol::getSymbolSite() const {
    return symbolSite;
}

ulong Symbol::getSymbolCnt() const {
    return symbolCnt;
}

bool Symbol::operator<(const Symbol &rhs) const {
    ulong left = num * rhs.getDen();
    ulong right = rhs.getNum() * den;

    return (left != right) ? (left < right) : (symbolSite < rhs.getSymbolSite()) ;

}

bool Symbol::operator==(const Symbol &rhs) const {
    return num == rhs.num &&
           den == rhs.den &&
           symbolSite == rhs.symbolSite &&
            symbolCnt == rhs.symbolCnt;
}

bool Symbol::isNull() const {
    return element == QChar(0) && den == 0;
}

QString Symbol::toString()
{
    QString res("");
    res.append(getElement());
    res+="";
//    res+= QString::number(getNum()) % "/" % QString::number(getDen()) % "-";
//    res+= QString::number(getSymbolSite()) % "." % QString::number(getSymbolCnt());
    return res;
}
