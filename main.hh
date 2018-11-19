#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QTimer>
#include <QHostInfo>
#include <QUuid>
#include <QDateTime>
#include <QtAlgorithms>

class NetSocket : public QUdpSocket {
    Q_OBJECT

public:
    NetSocket();

    // Bind this socket to a P2Papp-specific default port.
    bool bind();

public:
    quint16 myPortMin, myPortMax;
    quint16 port;
};

class ResponseTime {
public:
    ResponseTime() : portNum(0), sendTime(0), recvTime(0) {}

    ResponseTime(quint16 _portNum, qint64 _sendTime, qint64 _recvTime)
            : portNum(_portNum), sendTime(_sendTime), recvTime(_recvTime) {}

    void setSendTime(qint64 _sendTime) {
        sendTime = _sendTime;
    }

    void setRecvTime(qint64 _recvTime) {
        recvTime = _recvTime;
    }

    qint64 getResponseTime() const {
        return sendTime <= recvTime ? (recvTime - sendTime) : sendTime;
    }

    quint16 getPortNum() {
        return portNum;
    }

    bool operator<(const ResponseTime &rhs) const {
        return getResponseTime() < rhs.getResponseTime();
    }

private:
    quint16 portNum;
    qint64 sendTime, recvTime;
};

class ChatDialog : public QDialog {
    Q_OBJECT

public:
    ChatDialog();

private:
    void initResponseTime(quint16 portMin, quint16 portMax);

    quint16 findPort();

    void serializeMessage(
            QVariantMap message, QHostAddress destHost, quint16 destPort);

    void deserializeMessage(
            QByteArray datagram, QHostAddress senderHost, quint16 senderPort);

    void receiveRumorMessage(
            QVariantMap message, QHostAddress senderHost, quint16 senderPort);

    void receiveStatusMessage(
            QVariantMap message, QHostAddress senderHost, quint16 senderPort);

    void sendRumorMessage(
            QString origin,
            quint32 seqno,
            QHostAddress destHost,
            quint16 destPort);

    void sendStatusMessage(QHostAddress destHost, quint16 destPort);

    void rumor();

    QVariantMap buildRumorMessage(
            QString origin, quint32 seqno, QString charText);

    QVariantMap buildStatusMessage();

public slots:
    void gotReturnPressed();

    void receiveDatagrams();

    void rumorTimeout();

    void antiEntropyTimeout();

private:
    QTextEdit *textview;
    QLineEdit *textline;
    NetSocket *socket;
    QTimer *rumorTimer;
    QTimer *antiEntropyTimer;
    QString originName;
    quint16 portNum;
    QMap <QString, QStringList> messageDict;
    QString lastReceivedOrigin;
    quint32 lastReceivedSeqno;
    QMap <quint16, ResponseTime> responseTimeDict;
    static const int ANTI_ENTROPY_TIMEOUT = 4000;
    static const int RUMOR_TIMEOUT = 1000;
};

#endif // P2PAPP_MAIN_HH
