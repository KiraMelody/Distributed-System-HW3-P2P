
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog() {
    setWindowTitle("P2Papp");

    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    textview = new QTextEdit(this);
    textview->setReadOnly(true);

    // Small text-entry box the user can enter messages.
    // This widget normally expands only horizontally,
    // leaving extra vertical space for the textview widget.
    //
    // You might change this into a read/write QTextEdit,
    // so that the user can easily enter multi-line messages.
    textline = new QLineEdit(this);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:
    // http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(textview);
    layout->addWidget(textline);
    setLayout(layout);

    // Register a callback on the textline's returnPressed signal
    // so that we can send the message entered by the user.
    connect(textline, SIGNAL(returnPressed()),
            this, SLOT(gotReturnPressed()));
    connect(socket, SIGNAL(readyRead()),
			this, SLOT(receiveDatagrams()));
}

void ChatDialog::gotReturnPressed() {
    // Initially, just echo the string locally.
    // Insert some networking code here...
    qDebug() << "FIX: send message to other peers: " << textline->text();
    textview->append(textline->text());

    // process the message vis socket
    QString message = textline->text();
    if (messageDict.contains(QString(portNum))) {
    	QStringList myMessage = messageDict[QString(portNum)];
    	myMessage.insert(message);
    	messageDict[QString(portNum)] = myMessage;
    } else {
    	QStringList myMessage = (QStringList() << "");
    	myMessage.insert(message);
    	messageDict[QString(portNum)] = myMessage;
    }
	sendRumorMessage(QString(portNum), quint32(SeqNo));
	SeqNo += 1;

    // Clear the textline to get ready for the next input message.
    textline->clear();
}
void ChatDialog::receiveDatagrams()
{
	while (socket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(socket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;
	 
		if(socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort) != -1) {
			deserializeMessage(datagram);
		}
	}
}
void ChatDialog::findPort() {
	if (portNum == socket->myPortMax) {
		return portNum - 1;
	} else if (portNum == socket->myPortMin) {
		return portNum + 1;
	} else {
		if (qrand() % 2 == 0) {
			return portNum - 1;
		} else {
			return portNum + 1;
		}
	}
}

void ChatDialog::serializeMessage(QVariantMap message) {
    // To serialize a message you’ll need to construct a QVariantMap describing
    // the message
    //Create a byte array variable to store the byte string
	QByteArray datagram;
	QDataStream outStream(&datagram, QIODevice::ReadWrite); 
	outStream << message;
	
	socket->writeDatagram(datagram.data(), datagram.size(), QHostAddress::LocalHost, findPort());

	setTimeout();
}

void ChatDialog::deserializeMessage(QByteArray datagram) {
    // using QDataStream, and handle the message as appropriate
    // containing a ChatText key with a value of type QString
    QVariantMap message;
    QDataStream inStream(&datagram, QIODevice::ReadOnly);
    inStream >> message;
    if (message.contains("Want")) {
        receiveStatusMessage(message);
    } else {
        receiveRumorMessage(message);
    }
}

void ChatDialog::receiveRumorMessage(QVariantMap message) {
    // <”ChatText”,”Hi”> <”Origin”,”tiger”> <”SeqNo”,23>

    if (!message.contains("ChatText") ||
        !message.contains("Origin") ||
        !message.contains("SeqNo")) {
        // Invalid message.
        qDebug() << "WARNING: Received invalid rumor message!";
        return;
    }

    QString messageChatText = message["ChatText"].toString();
    QString messageOrigin = message["Origin"].toString();
    quint32 messageSeqNo = message["SeqNo"].toUint();

    if (messageDict.contains(messageOrigin) &&
        messageDict[messageOrigin].length() != messageSeqNo) {
        // skip duplicate and disorder.
        return;
    } else {
        textview->append(messageOrigin + ": ");
        textview->append(messageChatText);
        if (!messageDict.contains(messageOrigin)) {
            messageDict[messageOrigin] = (QStringList() << ""); // skip 0 index.
        }
        messageDict[messageOrigin].append(messageChatText);
    }
}

void ChatDialog::receiveStatusMessage(QVariantMap message) {
    // <"Want",<"tiger",4>> 4 is the message don't have

    QVariantMap statusMap = message['Want'];
    QList<QString> messageOriginList = statusMap.keys();
    for (QString origin: statusMap.keys()) {
    	quint32 seqno = statusMap[origin].value<quint32>();
    	if (messageDict.contains(origin)) {
    		quint32 last_seqno = messageDict[origin].size();
    		if (seqno > last_seqno) { // find this user need to update the message from origin
    			sendStatusMessage(origin, quint32(last_seqno + 1));
    		} else {				  // find sender need to update the message from origin
    			sendRumorMessage(origin, quint32(seqno + 1));
    		}

    	} else {
			sendStatusMessage(origin, quint32(0));
		}
    }
}

void ChatDialog::sendRumorMessage(Qstring origin, quint32 seqno) {
	QVariantMap message;
	if (messageDict[origin].size() > seqno)
	{	
		message.insert(QString("ChatText"), messageDict[origin].at(seqno));
		message.insert(QString("Origin"), QString(origin));
		message.insert(QString("SeqNo"), quint32(seqno));
	
		serializeMessage(message);
	}
}

void ChatDialog::sendStatusMessage(Qstring origin, quint32 seqno) {
	QVariantMap message;
	QVariantMap inner;

	inner.insert(origin, seqno);
	message.insert(Qstring('Want'), inner);
	serializeMessage(message);
}

void ChatDialog::setTimeout() {
    // Use QTimer
}

void ChatDialog::vectorClock() {
    // Use QTimer
}

NetSocket::NetSocket() {
    // Pick a range of four UDP ports to try to allocate by default,
    // computed based on my Unix user ID.
    // This makes it trivial for up to four P2Papp instances per user
    // to find each other on the same host,
    // barring UDP port conflicts with other applications
    // (which are quite possible).
    // We use the range from 32768 to 49151 for this purpose.
    myPortMin = 32768 + (getuid() % 4096) * 4;
    myPortMax = myPortMin + 3;
}

bool NetSocket::bind() {
    // Try to bind to each of the range myPortMin..myPortMax in turn.
    for (int p = myPortMin; p <= myPortMax; p++) {
        if (QUdpSocket::bind(p)) {
            qDebug() << "bound to UDP port " << p;
            return true;
        }
    }

    qDebug() << "Oops, no ports in my default range " << myPortMin
             << "-" << myPortMax << " available";
    return false;
}

int main(int argc, char **argv) {
    // Initialize Qt toolkit
    QApplication app(argc, argv);

    // Create an initial chat dialog window
    ChatDialog dialog;
    dialog.show();

    // Create a UDP network socket
    NetSocket sock;
    if (!sock.bind())
        exit(1);

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}

