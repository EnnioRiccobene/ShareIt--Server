#include "crdtprocessor.h"
#include "QList"
#include <QDebug>
#ifdef PROC_DEBUG

#endif

/////////////////////////////////////////////////////////////////////
//                         INIT METHODS
/////////////////////////////////////////////////////////////////////


CrdtProcessor::CrdtProcessor(QObject *parent, QString fileContent,
                             ulong siteId, ulong siteCnt) : QObject(parent), siteId(siteId), siteCnt(siteCnt){
regex_map = QRegularExpression(regex_map_pattern,QRegularExpression::DotMatchesEverythingOption|
                               QRegularExpression::UseUnicodePropertiesOption);
regex_operation = QRegularExpression(regex_operation_pattern,QRegularExpression::DotMatchesEverythingOption|
                                     QRegularExpression::UseUnicodePropertiesOption);
////////////////
#ifdef PROC_DEBUG
if(fileContent.isEmpty()){
    fileContent = CRDT_PROC_SERVER_MODE;
}
qDebug() << "fileContent: ->" << fileContent << "<-";
#endif
////////////////

regex_map.optimize();
regex_operation.optimize();


QList<Symbol> symbols;
try {
    if(!regex_map.match(fileContent,0,QRegularExpression::NormalMatch,
                              QRegularExpression::AnchoredMatchOption).hasMatch()){
        throw std::logic_error("data map parsing gone bad");
    }
   symbols  = CrdtProcessor::parseMap(fileContent);
} catch (std::logic_error) {
    Util::showErrorAndClose(PROC_ERR_MESSAGE);

}
if(!symbols.isEmpty()){
    for(auto it = symbols.begin(); it != symbols.end(); ++it){
        map.insert((*it),QString(it->getElement()));
        idMap.insert((*it),it->getSymbolSite());
    }
}

}


QList<Symbol> CrdtProcessor::parseMap(QString textMap){

    QList<Symbol> symbols;
    //MAP MESSAGE= ^'GS'PM'GS'('RS'<char>US<num>US<den>US<symSite>US<symCnt>)*'GS'PM'GS'$

    QStringList content = textMap.split(CRDT_PROCMAP_TAG,QString::SkipEmptyParts);
    if(!content.isEmpty()){
        QString body = content.first(); //retrieve symbols list separated by tag
        //parsing a symbol
        QStringList textSymbols = body.split(CRDT_RECORD_TAG,QString::SkipEmptyParts);
        for(auto it = textSymbols.begin(); it!= textSymbols.end(); ++it){
            QStringList fields = (*it).split(CRDT_SYMBOL_FIELD_TAG,QString::SkipEmptyParts);
            Symbol s= Symbol::parseSymbol(fields);
            symbols.push_back(s);
        }
    }
    return symbols;

}

/////////////////////////////////////////////////////////////////////
//    PROCESS LOCAL + SERVICE FUNCTIONS (INSERT/DELETE)
/////////////////////////////////////////////////////////////////////

QString CrdtProcessor::processLocal(QVector<ENUM_EDIT_CODE> operationType, QVector<int> positions, QVector<QChar> characters) {

    QString message(CRDT_PROCOP_TAG);
    if(operationType.size() != positions.size() || positions.size() != characters.size()){
        Util::showErrorAndClose(PROC_ERR_MESSAGE);

    }
    auto opIt = operationType.begin();
    auto posIt = positions.begin();
    auto chIt = characters.begin();
    bool correct=true;

        for(;correct && opIt != operationType.end() && posIt != positions.end() && chIt != characters.end(); ++opIt, ++posIt, ++chIt){
            if(*posIt<0){
                Util::showErrorAndClose(PROC_ERR_MESSAGE);
            }
            Symbol s = Symbol::nullSymbol();
            switch(*opIt) {

            case EDIT_CODE_INSERT:
                s = insertSymbol(*posIt,*chIt);
                if(s.isNull()){
                    correct=false;
                }
                else {
                    idMap.insert(s,s.getSymbolSite());
                    message.append(serializeOp(*opIt,s));
                }
                break;
            case EDIT_CODE_DELETE:
                s = deleteSymbol(*posIt);
                if(!s.isNull()){
                    idMap.remove(s);
                    message.append(serializeOp(*opIt,s));
                }
                break;
            default:
                correct=false;
                break;
            }

        }

    if(!correct){
        Util::showErrorAndClose(PROC_ERR_MESSAGE);
    }
    message.append(CRDT_PROCOP_TAG);

    ////////////////
    #ifdef PROC_DEBUG
    qDebug() << "operation created: ->" << message << "<-";
    #endif
    ////////////////

    return message;
}

Symbol CrdtProcessor::insertSymbol(int pos, QChar element){
    Symbol s = Symbol::nullSymbol();

    int size = map.size();
    if(size>=0){

        try {

            if(!size){
                //if map is empty, symbol is inserted as first at pos=0 regardless of the request
                s = Symbol::createSymbol(element,siteId,siteCnt++,INSERT_FIRST,s,s);
                map.insert(map.constBegin(),s,s.getElement());

            }
            //there exist at least 1 element in the document
            else{

                //insert head
                if(pos == 0){
                    s = Symbol::createSymbol(element,siteId,siteCnt++,INSERT_HEAD,s,*(map.keyBegin()));
                    map.insert(map.constBegin(),s,s.getElement());
                }
                //insert tail: positions greater than actual size are "trimmed" to tail insert
                else if(pos >= size){
                    s = Symbol::createSymbol(element,siteId,siteCnt++,INSERT_TAIL,(map.end()-1).key(),s);
                    map.insert(map.constEnd(),s,s.getElement());

                }
                //insert between (+ check request correctness)
                else if(size>=2 && pos >=1) {
                    auto itL = map.begin()+pos-1;
                    auto itR = map.begin()+pos;
                    s = Symbol::createSymbol(element,siteId,siteCnt++,INSERT_BETWEEN,itL.key(),itR.key());
                    map.insert(itR,s,s.getElement());

                }

            }
        } catch (std::logic_error) {
            return Symbol::nullSymbol();
        }

    }


    return s;
}


Symbol CrdtProcessor::deleteSymbol(int pos){
    int size = map.size();
    Symbol s = Symbol::nullSymbol();
    if(size <=0){
        return s;
    }
    auto it = map.begin();
    if(pos >= size-1){
        //re-align cursor to end
        it = map.end()-1;
        s = it.key();
        map.remove(s);
    }
    else {
        it+=pos;
        s = it.key();
        map.remove(s);
    }
    return s;

}

/////////////////////////////////////////////////////////////////////
//                     PROCESS REMOTE
/////////////////////////////////////////////////////////////////////

QVector<ulong> CrdtProcessor::processRemote(QString message) {

    ////////////////
    #ifdef PROC_DEBUG
    qDebug() << "received remote message: ->" << message << "<-";
    if(message.isEmpty()){
        message = CRDT_NOP;
        qDebug() << "debug message converted to: ->" << message << "<-";
    }

    #endif
    ////////////////

    if(!regex_operation.match(message,0,QRegularExpression::NormalMatch,
                              QRegularExpression::AnchoredMatchOption).hasMatch()){
        Util::showErrorAndClose(PROC_ERR_MESSAGE);
    }
    QVector<ulong> res(3);
    ulong minPos = 0, totIns = 0, totDelete = 0;
    QStringList content = message.split(CRDT_PROCOP_TAG,QString::SkipEmptyParts);
    if(!content.isEmpty()){
        QString body = content.first(); // (EDIT_CODE + SYMBOL)*
        QStringList operations = body.split(CRDT_RECORD_TAG,QString::SkipEmptyParts);
        for(auto opIt = operations.begin(); opIt != operations.end(); ++opIt){
            //process single operation
            QStringList fields = opIt->split(CRDT_SECTION_SEP,QString::SkipEmptyParts);
            uint opType = fields.first().toUInt();
            QStringList textSymbol = fields.last().split(CRDT_SYMBOL_FIELD_TAG,QString::SkipEmptyParts);
            Symbol s = Symbol::parseSymbol(textSymbol);
            bool correct = true;

            switch (opType) {

            case EDIT_CODE_INSERT:
                map.insert(s,QString(s.getElement()));
                idMap.insert(s,s.getSymbolSite());
                ++totIns;
                //exctract position of the first operation to get the minimum position
                if(opIt == operations.begin()){

                    int p = map.keys().indexOf(s);
                    if(p <0){
                        correct = false;
                    }
                    else{
                        minPos = static_cast<ulong>(qAbs(p));
                    }
                }
                break;
            case EDIT_CODE_DELETE:
                if(map.contains(s) && map.value(s)== QString(s.getElement())){
                    //exctract position of the first operation to get the minimum position
                    if(opIt == operations.begin()){

                        int p = map.keys().indexOf(s);
                        minPos = static_cast<ulong>(qAbs(p));

                    }
                    map.remove(s);
                    idMap.remove(s);
                    ++totDelete;
                }
                break;
            default:
                correct=false;
                break;
            }
            if(!correct){
                Util::showErrorAndClose(PROC_ERR_MESSAGE);
            }
        }
    }
    res[META_POS]=minPos;
    res[META_TOT_DELETE]=totDelete;
    res[META_TOT_INSERT]=totIns;
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SERIALIZE METHODS (Content: entire document / Op: list of operation executed on processLocal)
/////////////////////////////////////////////////////////////////////////////////////////////////

QString CrdtProcessor::serializeOp(ENUM_EDIT_CODE type, Symbol& s){
    return QString::number(type) % CRDT_SECTION_SEP % Symbol::serializeSymbol(s);
}


QString CrdtProcessor::serializeContent()
{
    QString res = CRDT_PROCMAP_TAG;
    QList<Symbol> keys = map.keys();
    for(auto it = keys.begin(); it != keys.end(); ++it){
        res+= Symbol::serializeSymbol(*it);
    }
    res+= CRDT_PROCMAP_TAG;
    return res;
}

/////////////////////////////////////////////////////////////////////
//                      GETTERS
/////////////////////////////////////////////////////////////////////


ulong CrdtProcessor::getSiteCnt() const{
    return siteCnt;
}

QList<ulong> CrdtProcessor::getSymIdList(){
    return idMap.values();
}

QString CrdtProcessor::printRemote(QString message)
{
    QString res("");
    if(!regex_operation.match(message,0,QRegularExpression::NormalMatch,
                              QRegularExpression::AnchoredMatchOption).hasMatch()){
        Util::showErrorAndClose(PROC_ERR_MESSAGE);
    }


    QStringList content = message.split(CRDT_PROCOP_TAG,QString::SkipEmptyParts);
    if(!content.isEmpty()){
        QString body = content.first(); // (EDIT_CODE + SYMBOL)*
        QStringList operations = body.split(CRDT_RECORD_TAG,QString::SkipEmptyParts);
        for(auto opIt = operations.begin(); opIt != operations.end(); ++opIt){
            //process single operation
            QString operation("");
            QStringList fields = opIt->split(CRDT_SECTION_SEP,QString::SkipEmptyParts);

            uint opType = fields.first().toUInt();
            QStringList textSymbol = fields.last().split(CRDT_SYMBOL_FIELD_TAG,QString::SkipEmptyParts);
            Symbol s = Symbol::parseSymbol(textSymbol);

            switch (opType) {

            case EDIT_CODE_INSERT:
                operation += "" % s.toString();
                res.append(operation);

                break;
            case EDIT_CODE_DELETE:
                operation += "" % s.toString();
                res.append(operation);

                break;
            default:
                operation += "NO!-" % s.toString();
                res.append(operation);
                break;
            }

        }
    }

    return res;
}


QString CrdtProcessor::getCurrentContentPlainText() {

    return QStringList(map.values()).join("");
}

