#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();
    void serializeMessage(QVariantMap message);
    void deserializeMessage(QByteArray datagram);
	void receiveRumorMessage(QVariantMap message);
	void receiveStatusMessage(QVariantMap message);
	void sendStatusMessage(Qstring origin, quint32 seqno);
	void sendRumorMessage(Qstring origin, quint32 seqno);
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

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a P2Papp-specific default port.
	bool bind();

private:
	int myPortMin, myPortMax;
};

#endif // P2PAPP_MAIN_HH
