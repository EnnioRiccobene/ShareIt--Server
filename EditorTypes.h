#ifndef EDITORTYPES_H
#define EDITORTYPES_H

#include <exception>
#include <QtGlobal>
#include <QChar>
#include <QString>
#include <QStringBuilder>
#include <QStringList>
#include <QVector>
#include <stdexcept>
#include <QRegularExpression>
#include <utility>
#include "util.h"

//#define PROC_DEBUG

enum ENUM_EDIT_CODE{EDIT_CODE_INSERT,EDIT_CODE_DELETE,EDIT_CODE_INVALID};
enum ENUM_RES_CODE{RES_CODE_OK,RES_CODE_EGENERAL};
enum ENUM_INSERT_TYPE{INSERT_HEAD,INSERT_BETWEEN,INSERT_TAIL,INSERT_FIRST,INSERT_INVALID};

enum ENUM_CRDT_REMOTE_META{META_POS,META_TOT_INSERT,META_TOT_DELETE};


#define ASCII_FS 28
#define ASCII_GS 29
#define ASCII_RS 30
#define ASCII_US 31

static QString CRDT_PROCMAP_TAG = QChar(ASCII_GS) % QString("PM") % QChar(ASCII_GS);
static QString CRDT_PROCOP_TAG = QChar(ASCII_FS) % QString("PO") % QChar(ASCII_FS);
static QString CRDT_RECORD_TAG = QChar(ASCII_RS);
static QString CRDT_SECTION_SEP = QChar(ASCII_GS);
static QString CRDT_SYMBOL_FIELD_TAG = QChar(ASCII_US);
static QString CRDT_PROC_SERVER_MODE = CRDT_PROCMAP_TAG % CRDT_PROCMAP_TAG;
static QString CRDT_NOP = CRDT_PROCOP_TAG % CRDT_PROCOP_TAG;

//regex
static QString regex_map_pattern = "^(\\x1DPM\\x1D)(\\X\\x1F(\\d+\\x1F){4}\\x1E)*((?1))$";
static QString regex_operation_pattern = "^(\\x1CPO\\x1C)(\\d\\x1D\\X\\x1F(\\d+\\x1F){4}\\x1E)*((?1))$";

#define PROC_ERR_MESSAGE "The program encountered an error during file processing \
\nand must be terminated\n"



#endif // EDITORTYPES_H
