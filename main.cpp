#include "widget.h"

#include <QApplication>
#include "networkmonitor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    int ret = a.exec();
    NetworkMonitor::clearNetworkMonitor();
    return ret;
}
