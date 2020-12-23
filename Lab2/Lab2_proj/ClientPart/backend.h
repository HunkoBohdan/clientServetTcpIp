#ifndef BACKEND_H
#define BACKEND_H

// secondary libraries
#include <QMessageBox>
#include <QObject>
// communication with server related
#include <QTcpSocket>
// JSON
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonDocument>
// for ukrainian language
#include <QTextCodec>
#include "QPushButton"

class BackEnd : public QObject
{
    Q_OBJECT
public:
    explicit BackEnd(QObject* parent = nullptr);
    QTcpSocket* socket;
    QByteArray recievedData;
    QJsonDocument* jsnDoc;
    QJsonParseError* errJsn = new QJsonParseError();
    QJsonObject* obj = new QJsonObject();;
    void decAndExec();
    void logProc();
    void regProc();
    void cListProc();
    void trainsListProc();
    void getAvailableSeats();

public slots:
    void createSocket();	// create and init socket with IP and port number
    void sendData(QByteArray dataToSend);
    void sockReady();	 // reading from socket
    void tryToReccon();
    void sockDisc();
    void buyNeededTicket();
signals:
    void _reconnFailed();
    void _reconnSuccess();
    void _logSuccess();
    void _regSuccess();
    void _errSignalMW(QString info);
    void _cList(QStringList cList);
    void _trainsList(QStringList trainsList);
    void _availableSeats(QString wagonsCounr, QStringList trainsList);
};

#endif	// BACKEND_H
