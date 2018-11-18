#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>

typedef QPair<QString, quint32> MessageKey;

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();
	void receiveRumorMessage(QVariantMap message);

public slots:
	void gotReturnPressed();

private:
	QTextEdit *textview;
	QLineEdit *textline;
	quint32 seqNo;
	quint16 portNum;
	QMap<MessageKey, QString> messageDict;
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
