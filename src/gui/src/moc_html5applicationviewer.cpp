/****************************************************************************
** Meta object code from reading C++ file 'html5applicationviewer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qt-daemon/html5applicationviewer/html5applicationviewer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'html5applicationviewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_Html5ApplicationViewer_t {
    QByteArrayData data[15];
    char stringdata[171];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Html5ApplicationViewer_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Html5ApplicationViewer_t qt_meta_stringdata_Html5ApplicationViewer = {
    {
QT_MOC_LITERAL(0, 0, 22),
QT_MOC_LITERAL(1, 23, 8),
QT_MOC_LITERAL(2, 32, 0),
QT_MOC_LITERAL(3, 33, 15),
QT_MOC_LITERAL(4, 49, 11),
QT_MOC_LITERAL(5, 61, 15),
QT_MOC_LITERAL(6, 77, 12),
QT_MOC_LITERAL(7, 90, 11),
QT_MOC_LITERAL(8, 102, 8),
QT_MOC_LITERAL(9, 111, 20),
QT_MOC_LITERAL(10, 132, 11),
QT_MOC_LITERAL(11, 144, 3),
QT_MOC_LITERAL(12, 148, 11),
QT_MOC_LITERAL(13, 160, 3),
QT_MOC_LITERAL(14, 164, 6)
    },
    "Html5ApplicationViewer\0do_close\0\0"
    "on_request_quit\0open_wallet\0generate_wallet\0"
    "close_wallet\0get_version\0transfer\0"
    "json_transfer_object\0message_box\0msg\0"
    "request_uri\0uri\0params"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Html5ApplicationViewer[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x08 /* Private */,
       3,    0,   60,    2, 0x08 /* Private */,
       4,    0,   61,    2, 0x0a /* Public */,
       5,    0,   62,    2, 0x0a /* Public */,
       6,    0,   63,    2, 0x0a /* Public */,
       7,    0,   64,    2, 0x0a /* Public */,
       8,    1,   65,    2, 0x0a /* Public */,
      10,    1,   68,    2, 0x0a /* Public */,
      12,    2,   71,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString,
    QMetaType::QString, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::QString, QMetaType::QString, QMetaType::QString,   13,   14,

       0        // eod
};

void Html5ApplicationViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Html5ApplicationViewer *_t = static_cast<Html5ApplicationViewer *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->do_close();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: { bool _r = _t->on_request_quit();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 2: _t->open_wallet(); break;
        case 3: _t->generate_wallet(); break;
        case 4: _t->close_wallet(); break;
        case 5: { QString _r = _t->get_version();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 6: { QString _r = _t->transfer((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 7: _t->message_box((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: { QString _r = _t->request_uri((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObject Html5ApplicationViewer::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Html5ApplicationViewer.data,
      qt_meta_data_Html5ApplicationViewer,  qt_static_metacall, 0, 0}
};


const QMetaObject *Html5ApplicationViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Html5ApplicationViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Html5ApplicationViewer.stringdata))
        return static_cast<void*>(const_cast< Html5ApplicationViewer*>(this));
    if (!strcmp(_clname, "view::i_view"))
        return static_cast< view::i_view*>(const_cast< Html5ApplicationViewer*>(this));
    return QWidget::qt_metacast(_clname);
}

int Html5ApplicationViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
