#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include "Dialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void sendMessage(); // 发送消息
    void readMessage(); // 接受信息
    void showMessage(bool isSend); // 显示消息

    void connectServer();  //连接服务器
    void haveconnected(); // 已连接状态
    void displayError(QAbstractSocket::SocketError); //显示错误

    void openFile();//打开文件
    void startTransferFile();  //发送文件大小等信息
    void updateClientProgress(qint64); //发送文件数据，更新进度条

    void updateServerProgress(); //接收数据，更新进度条

    void on_openButton_clicked();
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_sendMesButton_clicked();
    void on_sendFileButton_clicked();
    void on_helpButton_clicked();
    void on_openFolderButton_clicked();
    void on_quitButton_clicked();
    void on_textBrowser_textChanged();

private:
    Ui::MainWindow *ui;

    QUdpSocket *UdpSender;
    QUdpSocket *UdpReader;
    QString localMessage; // 存放本地要发送的信息
    QString serverMessage;  //存放从服务器接收到的信息

    QTcpSocket *tcpClient;

    quint16 blockSize;  //存放接收到的信息大小
    QFile *localFile;  //要发送的文件
    qint64 totalBytes;  //数据总大小
    qint64 bytesWritten;  //已经发送数据大小
    qint64 bytesToWrite;   //剩余数据大小
    qint64 loadSize;   //每次发送数据的大小
    QString fileName;  //保存文件路径
    QByteArray outBlock;  //数据缓冲区，即存放每次要发送的数据

    qint64 bytesReceived;  //已收到数据的大小
    qint64 fileNameSize;  //文件名的大小信息
    QByteArray inBlock;   //数据缓冲区

    Dialog *helpDialog;
    QFile *historyMessage;
    void setKeyvalue(bool connectKey, bool disconnectKey,bool openFileKey,bool sendFileKey);
};

#endif // MAINWINDOW_H
