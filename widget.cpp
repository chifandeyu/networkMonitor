#include "widget.h"
#include "ui_widget.h"
#include <QTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->label_wifi->setVisible(false);
    ui->label_Ethernet->setVisible(false);
    ui->label_error->setVisible(false);
    connect(NetworkMonitor::getNetworkMonitor(), &NetworkMonitor::sigNetworkStatus, this, &Widget::slotNetworkStatus);
    QTimer::singleShot(300, [this]() {
        NetworkMonitor::getNetworkMonitor()->ShowNetworkStatus();
    });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::slotNetworkStatus(NetworkType type, WiFiQuality quality)
{
    switch (type)
    {
    case NetworkType::NotNetwork:
        ui->label->setText(QString("network error"));
        ui->label_wifi->setVisible(false);
        ui->label_Ethernet->setVisible(false);
        ui->label_error->setVisible(true);
        break;
    case NetworkType::Ethernet:
        ui->label->setText(QString("connected Ethernet"));
        ui->label_wifi->setVisible(false);
        ui->label_Ethernet->setVisible(true);
        ui->label_error->setVisible(false);
        break;
    case NetworkType::Wlan:
    {
        ui->label_wifi->setVisible(true);
        ui->label_Ethernet->setVisible(false);
        ui->label_error->setVisible(false);
        QString strQuality;
        switch (quality)
        {
        case WiFiQuality::Weak:
            strQuality = tr("weak");
            break;
        case WiFiQuality::Fair:
            strQuality = tr("Fair");
            break;
        case WiFiQuality::Good:
            strQuality = tr("Good");
            break;
        case WiFiQuality::Excellent:
            strQuality = tr("Excellent");
            break;
        }
        ui->label->setText(QString("connected wifi, quality is %1").arg(strQuality));
    }
        break;
    default:
        break;
    }
}

