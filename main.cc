
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog() {
    setWindowTitle("P2Papp");

    socket = new NetSocket();
	if (!socket->bind()) {
		exit(1);
	} else {
		portNum = socket->port;
		originName =
		        QHostInfo.localHostName() + ": " + QVariant(portNum).toString();
        qDebug() << "origin name: " << originName;
        setWindowTitle(originName);
	}
    messageDict[originName] = (QStringList() << "");  // skip 0 index.
	lastReceivedSeqno = -1;
	lastReceivedOrigin = "";

    rumorTimer = new QTimer(this);
    antiEntropyTimer = new QTimer(this);
    antiEntropyTimer->start(ANTI_ENTROPY_TIMEOUT);
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
    connect(rumorTimer, SIGNAL(timeout()),
    		this, SLOT(rumorTimeout()));
    connect(antiEntropyTimer, SIGNAL(timeout()), 
    		this, SLOT(antiEntropyTimeout()));
}

void ChatDialog::gotReturnPressed() {
    // Initially, just echo the string locally.
    // Insert some networking code here...
    qDebug() << "FIX: send message to other peers: " << textline->text();

    // process the message vis socket
    QVariantMap message = buildRumorMessage(
            originName,
            messageDict[originName].size(),
            QString(textline->text()));
	receiveRumorMessage(message, QHostAddress::LocalHost, portNum);

    // Clear the textline to get ready for the next input message.
    textline->clear();
}

void ChatDialog::receiveDatagrams() {
    qDebug() << "receive datagram";
	while (socket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(socket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;
		if(socket->readDatagram(
		        datagram.data(),
		        datagram.size(),
		        &senderHost,
		        &senderPort) != -1) {
			deserializeMessage(datagram, senderHost, senderPort);
		}
	}
}

quint16 ChatDialog::findPort() {
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

void ChatDialog::serializeMessage(
        QVariantMap message, QHostAddress destHost, quint16 destPort) {
    // To serialize a message you’ll need to construct a QVariantMap describing
    // the message
    qDebug() << "serialize Message";
	QByteArray datagram;
	QDataStream outStream(&datagram, QIODevice::ReadWrite); 
	outStream << message;

	if (destPort < socket->myPortMin || destPort > socket->myPortMax) {
        destPort = findPort();
    }
    qDebug() << "Sending message to port: " << destPort;

	socket->writeDatagram(
	        datagram.data(),
	        datagram.size(),
            QHostAddress::LocalHost,
	        destPort);
}

void ChatDialog::deserializeMessage(
        QByteArray datagram, QHostAddress senderHost, quint16 senderPort) {
    // using QDataStream, and handle the message as appropriate
    // containing a ChatText key with a value of type QString
    qDebug() << "deserialize Message";
    QVariantMap message;
    QDataStream inStream(&datagram, QIODevice::ReadOnly);
    inStream >> message;
    if (message.contains("Want")) {
        receiveStatusMessage(message, senderHost, senderPort);
    } else {
        receiveRumorMessage(message, senderHost, senderPort);
    }
}

void ChatDialog::receiveRumorMessage(
        QVariantMap message, QHostAddress senderHost, quint16 senderPort) {
    // <”ChatText”,”Hi”> <”Origin”,”tiger”> <”SeqNo”,23>
    qDebug() << "receive RumorMessage";
    if (!message.contains("ChatText") ||
        !message.contains("Origin") ||
        !message.contains("SeqNo")) {
        // Invalid message.
        qDebug() << "WARNING: Received invalid rumor message!";
        return;
    }

    QString messageChatText = message["ChatText"].toString();
    QString messageOrigin = message["Origin"].toString();
    quint32 messageSeqNo = message["SeqNo"].toUInt();

    if (!messageDict.contains(messageOrigin)) {
        messageDict[messageOrigin] = (QStringList() << "");  // skip 0 index.
    }

    quint32 last_seqno = quint32(messageDict[messageOrigin].size());

    if (messageSeqNo == last_seqno) {
        if (messageOrigin != originName) {
            textview->append(messageOrigin + ": ");
        } else {
            textview->append(messageOrigin + "(me): ");
        }
        textview->append(messageChatText);
    	messageDict[messageOrigin].append(messageChatText);
    	sendStatusMessage(senderHost, senderPort);
    	lastReceivedOrigin = messageOrigin;
    	lastReceivedSeqno = messageSeqNo;
    	rumor();
    } else {
        sendStatusMessage(senderHost, senderPort);
    }
}

void ChatDialog::receiveStatusMessage(
        QVariantMap message, QHostAddress senderHost, quint16 senderPort) {
    // <"Want",<"tiger",4>> 4 is the message don't have
    qDebug() << "receive StatusMessage";
    if (!message.contains("Want")) {
        qDebug() << "Invalid StatusMessage";
        return;
    }
    rumorTimer->stop();
    QVariantMap statusVector = qvariant_cast<QVariantMap>(message["Want"]);

    bool isSame = true;
    bool isWant = false;
    QList<QString> keys = statusVector.keys();
    for (int i = 0; i < keys.size(); i++) {
        QString origin = keys[i];
    	quint32 seqno = statusVector[origin].value<quint32>();
    	if (messageDict.contains(origin)) {
    		quint32 last_seqno = quint32(messageDict[origin].size());
    		if (seqno > last_seqno) {
    		    // find this user need to update the message from origin
    			isSame = false;
    			isWant = true;
    		} else if (seqno < last_seqno) {
    		    // find sender need to update the message from origin
    		    isSame = false;
    		    for (quint32 curSeqno = seqno;
                     curSeqno < last_seqno;
                     curSeqno += quint32(1)) {
                    sendRumorMessage(origin, curSeqno, senderHost, senderPort);
    		    }
    		}
    	} else {
            messageDict[origin] = (QStringList() << "");  // skip 0 index.
            isWant = true;
    	}
    }
    if (isWant) {
        sendStatusMessage(senderHost, senderPort);
    }
    if (isSame) {
        if (qrand() % 2 == 0) {
            rumor();
        }
    }
}

void ChatDialog::sendRumorMessage(
        QString origin,
        quint32 seqno,
        QHostAddress destHost,
        quint16 destPort) {
    if (destPort == portNum) {
        qDebug() << "sending rumor to self port";
        return;
    }
    qDebug() << "sending RumorMessage from: " << origin << seqno;
    if (!messageDict.contains(origin) ||
        quint32(messageDict[origin].size()) <= seqno) {
        qDebug() << "invalid origin or seqno";
        return;
    }
	QVariantMap rumorMessage =
	        buildRumorMessage(origin, seqno, messageDict[origin].at(seqno));
    serializeMessage(rumorMessage, destHost, destPort);
}

void ChatDialog::sendStatusMessage(QHostAddress destHost, quint16 destPort) {
    if (destPort == portNum) {
        qDebug() << "sending rumor to self port";
        return;
    }
    qDebug() << "sending StatusMessage to: " << destPort;
	serializeMessage(buildStatusMessage(), destHost, destPort);
}

void ChatDialog::rumor() {
    if (lastReceivedOrigin == "" || lastReceivedSeqno <= 0) {
        return;
    }
    rumorTimer->start(RUMOR_TIMEOUT);
    sendRumorMessage(
            lastReceivedOrigin,
            lastReceivedSeqno,
            QHostAddress::LocalHost,
            findPort());
}

QVariantMap ChatDialog::buildRumorMessage(
        QString origin, quint32 seqno, QString charText) {
    QVariantMap rumorMessage;
    rumorMessage["ChatText"] = charText;
    rumorMessage["Origin"] = origin;
    rumorMessage["SeqNo"] = seqno;
    return rumorMessage;
}

QVariantMap ChatDialog::buildStatusMessage() {
    QVariantMap statusMessage;
    QVariantMap statusVector;
    QList<QString> keys = messageDict.keys();
    for (int i = 0; i < keys.size(); i++) {
        QString origin = keys[i];
        statusVector[origin] = quint32(messageDict[origin].size());
    }
    statusMessage["Want"] = statusVector;
    return statusMessage;
}

void ChatDialog::rumorTimeout() {
    // Use QTimer
    qDebug() << "rumor Timeout";
}

void ChatDialog::antiEntropyTimeout() {
    // Use QTimer
    qDebug() << "antiEntropy Timeout";
    antiEntropyTimer->start(ANTI_ENTROPY_TIMEOUT);
    quint16 randomPort = findPort();
    sendStatusMessage(QHostAddress::LocalHost, randomPort);
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
            port = p;
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

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
