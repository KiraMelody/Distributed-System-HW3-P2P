#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a P2Papp-specific default port.
	bool bind();

public:
	int myPortMin, myPortMax;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();
	quint16 findPort();
    void serializeMessage(QVariantMap message);
    void deserializeMessage(QByteArray datagram);
    void receiveDatagrams();
	void receiveRumorMessage(QVariantMap message);
	void receiveStatusMessage(QVariantMap message);
	void sendStatusMessage(QString origin, quint32 seqno);
	void sendRumorMessage(QString origin, quint32 seqno);
    void setTimeout();
    void vectorClock();

public slots:
	void gotReturnPressed();

private:
	QTextEdit *textview;
	QLineEdit *textline;
	NetSocket *socket;
	QTimer *timer;
	quint32 seqNo;
	quint16 portNum;
	QMap<QString, QStringList> messageDict;
};

#endif // P2PAPP_MAIN_HH
