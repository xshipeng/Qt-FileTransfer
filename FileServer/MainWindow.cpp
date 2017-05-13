#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QObject>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    UdpSender = new QUdpSocket(this);
    UdpReader = new QUdpSocket(this);

  
    UdpReader->bind(8181, QUdpSocket::ShareAddress);  // 将UdpReader绑定到广播地址的8181端口，接收发到这个端口的信息
    connect(UdpReader, SIGNAL(readyRead()), this, SLOT(readMessage()));

    loadSize = 4*1024;  //将整个大的文件分成很多小的部分进行发送，每部分为4字节
    totalBytes = 0;//总文件大小
    bytesWritten = 0;//已经写入的文件大小
    bytesToWrite = 0;//要写入的文件
    bytesReceived = 0;//已经收到的文件大小
    fileNameSize = 0;//文件名的大小

    tcpServer = new QTcpServer(this);
    tcpSocket = new QTcpSocket(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));//当发现新连接时发出newConnection()信号

    ui->sendMesButton->setEnabled(true);
    setKeyvalue(true,false,false,false);//批量设置按钮可用性

    getHostIP();
    ui->portLineEdit->setText("9628");//设置默认的端口号

    helpDialog = new Dialog;
    helpDialog->hide();//初始化一个新的帮助界面并且隐藏

    historyMessage = new QFile("historyMessage.txt");
    if(!historyMessage->open(QFile::Append))
    {
        qDebug() << "写入历史文件错误";
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::listening()// 监听
{
    loadSize = 4*1024;  //将整个大的文件分成很多小的部分进行发送，每部分为4字节
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    blockSize = 0; //初始化其为0

    if(!tcpServer->listen(QHostAddress::LocalHost, ui->portLineEdit->text().toInt()))
    {
        qDebug() << tcpServer->errorString();
        close();
        return;
    }

    ui->currentStatusLabel->setText(u8"正在等待连接");
}

// 连接后的对话框提示
void MainWindow::acceptConnection()
{
    setKeyvalue(false,true,true,false);//批量设置按钮可用性
    tcpSocket = tcpServer->nextPendingConnection();

    // 当有数据发送成功时，更新进度条并继续发送
    connect(tcpSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));//通过槽函数传递参数

    // 当有数据接收成功时，更新进度条并继续接收
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(updateServerProgress()));

    // 绑定错误处理
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));

    tcpServer->close();

    ui->currentStatusLabel->setText(u8"会话建立成功");
}

// 发送信息
void MainWindow::sendMessage()
{
    localMessage = ui->inputTextEdit->toPlainText();

    QByteArray datagram = localMessage.toUtf8();
    UdpSender->writeDatagram(datagram.data(), datagram.size(), QHostAddress::Broadcast, 7171);//进行UDP数据报（datagrams）的发送

    // 显示消息
    showMessage(true);
}

// 接收信息
void MainWindow::readMessage()
{
    //拥有等待的数据报
    while(UdpReader->hasPendingDatagrams())
    {
        QByteArray datagram; //拥于存放接收的数据报

        //让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
        datagram.resize(UdpReader->pendingDatagramSize());

        //接收数据报，将其存放到datagram中
        UdpReader->readDatagram(datagram.data(), datagram.size());

        //将数据报内容显示出来
        serverMessage = datagram;
    }
    // 显示消息
    showMessage(false);
}

// 显示消息
void MainWindow::showMessage(bool isSend)
{
    QDateTime time = QDateTime::currentDateTime();  //获取系统现在的时间
    QString str = time.toString("hh:mm:ss ddd");     //设置显示格式
    QString str4file = time.toString("yyyy-MM-dd hh:mm:ss");     //设置显示格式
    blockSize = 0;

    QFont font;
    font.setPixelSize(18);
    ui->textBrowser->setFont(font);

    QTextStream stream(historyMessage);

    if (isSend)
    {
        // 用不同的颜色显示信息所属和当前时间
        ui->textBrowser->setTextColor(QColor(0, 0, 0));
        QString entraMess = u8"我: " + str;
        ui->textBrowser->append(entraMess);
        stream<<"我: "<<str4file<<"\n"; // 写入历史信息文件时需要保存日期
        ui->textBrowser->setTextColor(QColor(0, 0, 255));
        ui->textBrowser->append(localMessage);
        ui->inputTextEdit->clear();//清除对话框的消息显示
        ui->currentStatusLabel->setText(u8"消息发送成功");
        stream<<localMessage<<"\n";
    }
    else
    {
        // 用不同的颜色显示信息所属和当前时间
        ui->textBrowser->setTextColor(QColor(0, 0, 0));
        QString entraMess = u8"对方: " + str;
        ui->textBrowser->append(entraMess);
        stream<<"对方: "<<str4file<<"\n";

        ui->textBrowser->setTextColor(QColor(255, 0, 0));
        ui->textBrowser->append(serverMessage);
        ui->currentStatusLabel->setText(u8"有新消息到达");
        stream<<localMessage<<"\n";
    }
}

// 打开文件
void MainWindow::openFile()
{
    fileName = QFileDialog::getOpenFileName(this);
    if(!fileName.isEmpty())
    {
        QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
        ui->currentStatusLabel->setText(tr(u8"成功打开文件 %1").arg(currentFileName));
        setKeyvalue(false,true,true,true);//批量设置按钮可用性
    }
}

// 实现文件大小等信息的发送
void MainWindow::startTransferFile()
{
    setKeyvalue(false,false,false,false);//批量设置按钮可用性
    localFile = new QFile(fileName);
    if(!localFile->open(QFile::ReadOnly))
    {
        qDebug() << "打开文件出错";
        return;
    }
    totalBytes = localFile->size(); //文件实际总大小

    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);//将发送缓冲区outBlock封装在一个QDataStream类型的变量
    sendOut.setVersion(QDataStream::Qt_5_0);
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);//通过QString类的right()函数去掉文件的路径部分，仅将文件部分保存在currentFile变量中
    sendOut << qint64(0) << qint64(0) << currentFileName;//文件头结构由三个字段组成，分别是64位的总长度(包括文件数据长度和文件头自身长度)，64位的文件名长度和文件名。

    totalBytes += outBlock.size();//这里的总大小是文件名大小等信息和实际文件大小的总和
    sendOut.device()->seek(0);//函数将读写操作指向从头开始
    sendOut<<totalBytes<<qint64((outBlock.size() - sizeof(qint64)*2));//用实际的总长度大小信息和文件名长度信息代替两个qint64(0)空间
    bytesToWrite = totalBytes - tcpSocket->write(outBlock);//发送完头数据后剩余数据的大小
    ui->currentStatusLabel->setText(tr(u8"正在发送%1").arg(currentFileName));
    outBlock.resize(0);//清空发送缓冲区以备下次使用
}

// 更新进度条，实现文件的传送
void MainWindow::updateClientProgress(qint64 numBytes)
{
    //已经发送数据的大小
    bytesWritten += (int)numBytes;
    if(bytesToWrite > 0) //如果已经发送了数据
    {
        outBlock = localFile->read(qMin(bytesToWrite, loadSize));//每次发送loadSize大小的数据，这里设置为4KB，如果剩余的数据不足4KB，就发送剩余数据的大小
        bytesToWrite -= (int)tcpSocket->write(outBlock);//发送完一次数据后还剩余数据的大小

        //清空发送缓冲区
        outBlock.resize(0);
    }
    else
    {
        //如果没有发送任何数据，则关闭文件
        localFile->close();
    }

    //更新进度条
    ui->serverProgressBar->setMaximum(totalBytes);
    ui->serverProgressBar->setValue(bytesWritten);

    if(bytesWritten == totalBytes) //发送完毕
    {
        QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
        ui->currentStatusLabel->setText(tr(u8"成功发送文件%1 ...").arg(currentFileName));
        setKeyvalue(false,true,true,false);//批量设置按钮可用性
        outBlock.resize(0);
        localFile->close();
        bytesWritten = 0; // 为下次发送做准备
        bytesToWrite = 0;
        totalBytes = 0;
}
    }

// 更新进度条，实现文件的接收
//从TCP数据流中接收前16个字节(两个qint64结构长)，用来确定总共需接收的字节数和文件名长度，并将这两个值保存在私有成员TotalBytes和fileNameSize中，然后根据fileNameSize值接收文件名。
void MainWindow::updateServerProgress()
{
    QDataStream sendIn(tcpSocket);
    sendIn.setVersion(QDataStream::Qt_5_0);
    if(bytesReceived <= sizeof(qint64)*2)  //如果接收到的数据小于等于16个字节，那么是刚开始接收数据，保存为头文件信息
    {
        //接收数据总大小信息和文件名大小信息
        if((tcpSocket->bytesAvailable() >= sizeof(qint64)*2) && (fileNameSize == 0))//确保至少有16字节的可用数据且文件名长度为0(表示未从TCP连接接收文件名长度字段，仍处于第一步操作)
        {
            sendIn >> totalBytes >> fileNameSize;//读取总共需接收的数据和文件名长度
            bytesReceived += sizeof(qint64) * 2;
        }

        //接收文件名，并建立文件
        if((tcpSocket->bytesAvailable() >= fileNameSize) && (fileNameSize != 0))//确保连接上的数据已包含完整的文件名且文件名长度不为0(表示已从TCP连接接收文件名长度字段，处于第二步操作中)
        {
            sendIn >> fileName;//读取文件名
            ui->currentStatusLabel->setText(tr(u8"正在接收文件 %1 ...").arg(fileName));
            setKeyvalue(false,false,false,false);//批量设置按钮可用性
            bytesReceived += fileNameSize;

            localFile = new QFile(fileName);
            if(!localFile->open(QFile::WriteOnly))
            {
                qDebug() << "写入文件错误";
                return;
            }
        }
        else
            return;

    }

    //如果接收的数据小于总数据，那么写入文件
    if(bytesReceived < totalBytes)
    {
        bytesReceived += tcpSocket->bytesAvailable();
        inBlock = tcpSocket->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    //更新进度条
    ui->serverProgressBar->setMaximum(totalBytes);
    ui->serverProgressBar->setValue(bytesReceived);

    //接收数据完成时
    if(bytesReceived == totalBytes)
    {
        localFile->close();
        setKeyvalue(false,true,true,false);//批量设置按钮可用性
        ui->currentStatusLabel->setText(tr(u8"成功接收文件 %1").arg(fileName));
        bytesReceived = 0; // 为下次传输初始化
        totalBytes = 0;
        fileNameSize = 0;
    }
}

// 显示错误
void MainWindow::displayError(QAbstractSocket::SocketError)
{
    qDebug() << tcpSocket->errorString();
    tcpSocket->close();
    ui->serverProgressBar->reset();
    ui->currentStatusLabel->setText(tr(u8"连接错误，请重试"));
    setKeyvalue(true,false,false,false);//批量设置按钮可用性
    loadSize = 4*1024;  //将整个大的文件分成很多小的部分进行发送，每部分为4字节
    totalBytes = 0;//总文件大小
    bytesWritten = 0;//已经写入的文件大小
    bytesToWrite = 0;//要写入的文件
    bytesReceived = 0;//已经收到的文件大小
    fileNameSize = 0;//文件名的大小
    outBlock.resize(0);
    inBlock.resize(0);
//    localFile->close();
}

// 开始监听
void MainWindow::on_listenButton_clicked()
{
    listening();
    setKeyvalue(false,true,false,false);//批量设置按钮可用性
}

// 断开连接
void MainWindow::on_disconnectButton_clicked()
{
    tcpSocket->disconnectFromHost();//如果连接已经建立，则断开连接
    tcpSocket->readAll();//清空缓存区，为下一次连接做准备
    tcpServer->close();//如果服务器正在监听，则停止监听
    setKeyvalue(true,false,false,false);//批量设置按钮可用性
    loadSize = 4*1024;  //将整个大的文件分成很多小的部分进行发送，每部分为4字节
    totalBytes = 0;//总文件大小
    bytesWritten = 0;//已经写入的文件大小
    bytesToWrite = 0;//要写入的文件
    bytesReceived = 0;//已经收到的文件大小
    fileNameSize = 0;//文件名的大小
    ui->serverProgressBar->reset();
    outBlock.resize(0);
     inBlock.resize(0);
//    localFile->close();
}

// 发送信息
void MainWindow::on_sendMesButton_clicked()
{
    sendMessage();
}
// 打开文件夹
void MainWindow::on_openFolderButton_clicked()
{
    QString path=QDir::currentPath();//获取程序当前目录
    path.replace("/","\\");//将地址中的"/"替换为"\"，因为在Windows下使用的是"\"。
    QProcess::startDetached("explorer "+path);//打开上面获取的目录
}
// 打开文件
void MainWindow::on_openButton_clicked()
{
    openFile();
}

// 发送文件
void MainWindow::on_sendFileButton_clicked()
{
    startTransferFile();
}

// 帮助说明
void MainWindow::on_helpButton_clicked()
{
    helpDialog->show();
}

// 关闭程序
void MainWindow::on_quitButton_clicked()
{
    this->close();
}

// 文字多了会增加滚动条并显示最后的信息
void MainWindow::on_textBrowser_textChanged()
{
    ui->textBrowser->moveCursor(QTextCursor::End);
}

QString MainWindow::getHostIP()//获取本机的IP地址
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
        //我们使用IPv4地址
       if(address.protocol() == QAbstractSocket::IPv4Protocol)
           ui->comboBox->addItem(address.toString());//服务器存在于多个网段，将所有的IP地址放入ComboBox下拉框供用户选择
    }
    return 0;
}

void MainWindow::setKeyvalue(bool connectKey, bool disconnectKey,bool openFileKey,bool sendFileKey)
{
    ui->listenButton->setEnabled(connectKey);
    ui->disconnectButton->setEnabled(disconnectKey);
    ui->openButton->setEnabled(openFileKey);
    ui->sendFileButton->setEnabled(sendFileKey);

}


