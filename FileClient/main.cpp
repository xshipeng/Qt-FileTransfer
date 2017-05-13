#include "MainWindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));//防止中文字体乱码
    QApplication a(argc, argv);//Qapplication对象
    MainWindow w;//创建MainWindow的窗口
    w.show();
    return a.exec();//进入消息循环，等待可能的输入进行响应，将main()函数的控制权交给Qt
}
