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

public slots:
	void gotReturnPressed();

private:
	QTextEdit *textview;
	QLineEdit *textline;
	quint32 seqNo;
	quint16 portNum;
	QMap<quint32, QString> messageQueue;
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
