#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QTimer>

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a P2Papp-specific default port.
	bool bind();

public:
	quint16 myPortMin, myPortMax;
	quint16 port;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();
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
	QMap<QString, QStringList> messageDict;
    QString lastReceivedOrigin;
    quint32 lastReceivedSeqno;
    static const ANTI_ENTROPY_TIMEOUT = 4000;
    static const RUMOR_TIMEOUT = 1000;
};

#endif // P2PAPP_MAIN_HH
