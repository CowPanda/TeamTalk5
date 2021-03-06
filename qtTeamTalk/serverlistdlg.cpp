/*
 * Copyright (c) 2005-2014, BearWare.dk
 * 
 * Contact Information:
 *
 * Bjoern D. Rasmussen
 * Skanderborgvej 40 4-2
 * DK-8000 Aarhus C
 * Denmark
 * Email: contact@bearware.dk
 * Phone: +45 20 20 54 59
 * Web: http://www.bearware.dk
 *
 * This source code is part of the TeamTalk 5 SDK owned by
 * BearWare.dk. All copyright statements may not be removed 
 * or altered from any source distribution. If you use this
 * software in a product, an acknowledgment in the product 
 * documentation is required.
 *
 */

#include "serverlistdlg.h"
#include "common.h"
#include "appinfo.h"
#include "settings.h"
#include "generatettfiledlg.h"

#include <QUrl>
#include <QMessageBox>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkReply>

extern QSettings* ttSettings;

ServerListDlg::ServerListDlg(QWidget * parent/* = 0*/)
    : QDialog(parent, QT_DEFAULT_DIALOG_HINTS)
    , m_http_manager(NULL)
{
    ui.setupUi(this);
    setWindowIcon(QIcon(APPICON));

#ifndef ENABLE_ENCRYPTION
    ui.cryptChkBox->hide();
#endif

    connect(ui.addupdButton, SIGNAL(clicked()),
            SLOT(slotAddUpdServer()));
    connect(ui.delButton, SIGNAL(clicked()),
            SLOT(slotDeleteServer()));
    connect(ui.listWidget, SIGNAL(currentRowChanged(int)),
            SLOT(slotShowServer(int)));
    connect(ui.connectButton, SIGNAL(clicked()),
            SLOT(slotConnect()));
    connect(ui.clearButton, SIGNAL(clicked()),
            SLOT(slotClearServerClicked()));
//    connect(ui.listWidget, SIGNAL(itemChanged(QListWidgetItem*)),
//            SLOT(slotServerSelected(QListWidgetItem*)));
    connect(ui.listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            SLOT(slotDoubleClicked(QListWidgetItem*)));
    connect(ui.freeserverChkBox, SIGNAL(clicked(bool)),
            SLOT(slotFreeServers(bool)));
    connect(ui.genttButton, SIGNAL(clicked()),
            SLOT(slotGenerateFile()));
    connect(ui.hostaddrBox, SIGNAL(currentIndexChanged(int)),
            SLOT(slotShowHost(int)));
    connect(ui.nameEdit, SIGNAL(textChanged(const QString&)),
            SLOT(slotSaveEntryChanged(const QString&)));

    connect(ui.hostaddrBox, SIGNAL(editTextChanged(const QString&)),
            SLOT(slotGenerateEntryName(const QString&)));
    connect(ui.tcpportEdit, SIGNAL(textChanged(const QString&)),
            SLOT(slotGenerateEntryName(const QString&)));
    connect(ui.usernameEdit, SIGNAL(textChanged(const QString&)),
            SLOT(slotGenerateEntryName(const QString&)));

    clearServer();

    HostEntry host;
    int index = 0;
    while(getLatestHost(index++, host))
        ui.hostaddrBox->addItem(host.ipaddr);
    slotShowHost(0);

    if(ttSettings->value(SETTINGS_DISPLAY_FREESERVERS, true).toBool())
        ui.freeserverChkBox->setChecked(true);

    ui.delButton->setEnabled(false);
    showServers();
}

void ServerListDlg::slotClearServerClicked()
{
    clearServer();
    ui.hostaddrBox->setFocus();
}

void ServerListDlg::clearServer()
{
    ui.nameEdit->setText("");
    ui.hostaddrBox->lineEdit()->setText("");
    ui.tcpportEdit->setText(QString::number(DEFAULT_TCPPORT));
    ui.udpportEdit->setText(QString::number(DEFAULT_UDPPORT));
    ui.cryptChkBox->setChecked(false);
    ui.usernameEdit->setText("");
    ui.passwordEdit->setText("");
    ui.channelEdit->setText("");
    ui.chanpasswdEdit->setText("");

    ui.clearButton->setEnabled(false);
}

void ServerListDlg::slotShowHost(int index)
{
    HostEntry host;
    if(getLatestHost(index, host))
    {
        showHost(host);
        ui.delButton->setEnabled(false);
    }
}

void ServerListDlg::showHost(const HostEntry& entry)
{
    ui.nameEdit->setText(entry.name);
    ui.hostaddrBox->lineEdit()->setText(entry.ipaddr);
    ui.tcpportEdit->setText(QString::number(entry.tcpport));
    ui.udpportEdit->setText(QString::number(entry.udpport));
    ui.cryptChkBox->setChecked(entry.encrypted);
    ui.usernameEdit->setText(entry.username);
    ui.passwordEdit->setText(entry.password);
    ui.channelEdit->setText(entry.channel);
    ui.chanpasswdEdit->setText(entry.chanpasswd);

    ui.clearButton->setEnabled(true);
}

void ServerListDlg::showServers()
{
    m_servers.clear();
    ui.listWidget->clear();
    int index = 0;
    HostEntry entry;
    while(getServerEntry(index++, entry))
        m_servers.push_back(entry);

    for(int i=0;i<m_servers.size();i++)
        ui.listWidget->addItem(m_servers[i].name);

    if(ui.freeserverChkBox->isChecked())
        slotFreeServers(true);
}

void ServerListDlg::slotShowServer(int index)
{
    clearServer();
    if(index >= 0 && index < m_servers.size())
    {
        showHost(m_servers[index]);
        ui.delButton->setEnabled(true);
    }
    else
    {
        ui.clearButton->setEnabled(false);
        ui.delButton->setEnabled(false);
    }
}

void ServerListDlg::slotAddUpdServer()
{
    HostEntry entry;
    if(getHostEntry(entry))
    {
        deleteServerEntry(entry.name);
        addServerEntry(entry);
        showServers();
    }
}

void ServerListDlg::slotDeleteServer()
{
    QListWidgetItem* item = ui.listWidget->currentItem();
    if(item)
    {
        deleteServerEntry(item->text());
        clearServer();
        showServers();
        ui.delButton->setEnabled(false);
    }
}

bool ServerListDlg::getHostEntry(HostEntry& entry)
{
    if(ui.hostaddrBox->lineEdit()->text().isEmpty() ||
       ui.tcpportEdit->text().isEmpty() ||
       ui.udpportEdit->text().isEmpty())
    {
        QMessageBox::information(this, tr("Missing fields"),
            tr("Please fill the fields 'Host IP-address', 'TCP port' and 'UDP port'"));
            return false;
    }

    entry.name = ui.nameEdit->text();
    entry.ipaddr = ui.hostaddrBox->lineEdit()->text();
    entry.tcpport = ui.tcpportEdit->text().toInt();
    entry.udpport = ui.udpportEdit->text().toInt();
    entry.encrypted = ui.cryptChkBox->isChecked();
    entry.username = ui.usernameEdit->text();
    entry.password = ui.passwordEdit->text();
    entry.channel = ui.channelEdit->text();
    entry.chanpasswd = ui.chanpasswdEdit->text();

    return true;
}

void ServerListDlg::slotConnect()
{
    HostEntry entry;
    if(getHostEntry(entry))
    {
        addLatestHost(entry);
        this->accept();
    }
}

void ServerListDlg::slotServerSelected(QListWidgetItem * item)
{
    Q_UNUSED(item);
    qDebug() << "Activated";
}

void ServerListDlg::slotDoubleClicked(QListWidgetItem*)
{
    slotConnect();
}

void ServerListDlg::slotFreeServers(bool checked)
{
    ttSettings->setValue(SETTINGS_DISPLAY_FREESERVERS, checked);

    if(!checked)
    {
        showServers();
        return;
    }
    if(!m_http_manager)
        m_http_manager = new QNetworkAccessManager(this);

    QUrl url(URL_FREESERVER);
    connect(m_http_manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(slotFreeServerRequest(QNetworkReply*)));

    QNetworkRequest request(url);
    m_http_manager->get(request);
    //QByteArray path = QUrl::toPercentEncoding(url.path(), "!$&'()*+,;=:@/");
    //if (path.isEmpty())
    //    path = "/";
}

void ServerListDlg::slotFreeServerRequest(QNetworkReply* reply)
{
    Q_ASSERT(m_http_manager);
    QByteArray data = reply->readAll();

    QDomDocument doc("foo");
    if(!doc.setContent(data))
        return;

    int index = m_servers.size();
	QDomElement rootElement(doc.documentElement());
	QDomElement element = rootElement.firstChildElement();
    while(!element.isNull())
    {
        HostEntry entry;
        if(getServerEntry(element, entry))
            m_servers.push_back(entry);
		element = element.nextSiblingElement();
    }
    for(;index<m_servers.size();index++)
    {
        QListWidgetItem* srvItem = new QListWidgetItem(ui.listWidget);
        srvItem->setText(m_servers[index].name);
        srvItem->setBackgroundColor(QColor(133,229,141));
        ui.listWidget->addItem(srvItem);
    }
}

void ServerListDlg::slotGenerateFile()
{
    HostEntry entry;
    if(!getHostEntry(entry))
        return;

    GenerateTTFileDlg dlg(entry, this);
    dlg.exec();
}

void ServerListDlg::slotSaveEntryChanged(const QString& text)
{
    ui.addupdButton->setEnabled(text.size());
}

void ServerListDlg::slotGenerateEntryName(const QString&)
{
    QString username = ui.usernameEdit->text();
    if(username.size())
        ui.nameEdit->setText(QString("%1@%2:%3")
                             .arg(username)
                             .arg(ui.hostaddrBox->lineEdit()->text())
                             .arg(ui.tcpportEdit->text()));
    else if(ui.hostaddrBox->lineEdit()->text().size())
        ui.nameEdit->setText(QString("%1:%2")
                             .arg(ui.hostaddrBox->lineEdit()->text())
                             .arg(ui.tcpportEdit->text()));
    else
        ui.nameEdit->setText(QString());
}
