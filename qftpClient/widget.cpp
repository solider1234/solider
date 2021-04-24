#include "widget.h"
#include "ui_widget.h"

#include <QAbstractSocket>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QString>
#include <QThread>
#include <QTimer>
static Qt::ItemFlags Flags = Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable;
static int level = 1;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setWindowTitle("ftpClient");
    ftp = new QFtp(this);
    connect(ftp, SIGNAL(done(bool)), this, SLOT(doneSlot(bool)));
    connect(ftp, SIGNAL(commandFinished(int,bool)),this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftp, SIGNAL(commandStarted(int)),this, SLOT(commandStartedSlot(int)));
    connect(ftp, SIGNAL(stateChanged(int)),this, SLOT(stateChangedSlot(int)));
    connect(ftp, SIGNAL(readyRead()),this, SLOT(readyReadSlot()));
    connect(ftp, SIGNAL(listInfo(const QUrlInfo &)),this, SLOT(listInfoSlot(const QUrlInfo &)));
    connect(ftp, SIGNAL(dataTransferProgress(qint64, qint64)),this, SLOT(dataTransferProgressSlot(qint64, qint64)));

//    ui->treeWidget->setSortingEnabled(true);
//    ui->treeWidget->header()->setSortIndicatorShown(true);
//    ui->treeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);


    m_cmdMap.insert(int(QFtp::None),QString::fromLocal8Bit("cmd: 无"));
    m_cmdMap.insert(int(QFtp::SetTransferMode),QString::fromLocal8Bit("cmd: 设置传输模式"));
    m_cmdMap.insert(int(QFtp::SetProxy),QString::fromLocal8Bit("cmd: 切换代理打开或关闭"));
    m_cmdMap.insert(int(QFtp::ConnectToHost),QString::fromLocal8Bit("cmd: 执行connectToHost()"));
    m_cmdMap.insert(int(QFtp::Login),QString::fromLocal8Bit("cmd: 执行login()"));
    m_cmdMap.insert(int(QFtp::Close),QString::fromLocal8Bit("cmd: 执行close()"));
    m_cmdMap.insert(int(QFtp::List),QString::fromLocal8Bit("cmd: 列出目录下的文件"));
    m_cmdMap.insert(int(QFtp::Cd),QString::fromLocal8Bit("cmd: 执行cd()"));
    m_cmdMap.insert(int(QFtp::Get),QString::fromLocal8Bit("cmd: 执行Get()"));
    m_cmdMap.insert(int(QFtp::Put),QString::fromLocal8Bit("cmd: 执行Put()"));
    m_cmdMap.insert(int(QFtp::Remove),QString::fromLocal8Bit("cmd: 执行Remove()"));
    m_cmdMap.insert(int(QFtp::Mkdir),QString::fromLocal8Bit("cmd: 执行Mkdir()"));
    m_cmdMap.insert(int(QFtp::Rmdir),QString::fromLocal8Bit("cmd: 执行Rmdir()"));
    m_cmdMap.insert(int(QFtp::Rename),QString::fromLocal8Bit("cmd: 执行Rename()"));
    m_cmdMap.insert(int(QFtp::RawCommand),QString::fromLocal8Bit("cmd:执行RawCommand()"));


    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setHidden(true);

    QStringList header;
    header << QString::fromLocal8Bit("fileType")<<QString::fromLocal8Bit("fileName") << QString::fromLocal8Bit("size(bytes)");
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels(header);

    ui->tableWidget->insertRow(0);
    QPushButton *m_backtoLastbtn = new QPushButton;
    m_backtoLastbtn->setStyleSheet("border:none;");
    ui->tableWidget->setCellWidget(0,0,(QWidget *)m_backtoLastbtn);
    ui->tableWidget->setItem(0, 1, new QTableWidgetItem(".."));
    ui->tableWidget->setItem(0, 2, new QTableWidgetItem("  "));
    m_backtoLastbtn->setIcon(QIcon(":/image/cdtoparent.png"));

//    m_backtoLastbtn->setStyleSheet("border:none;border-image: url(:/image/cdtoparent.png);");
    ui->lineEditPassWord->installEventFilter(this);
//    ui->lineEditCD->installEventFilter(this);

    this->setAcceptDrops(true);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->lineEditPassWord)
    {
        if(event->type() == QEvent::FocusIn)
        {
            ui->lineEditPassWord->setEchoMode(QLineEdit::Normal);
            return false;
        }
        else if(event->type() == QEvent::FocusOut)
        {
            ui->lineEditPassWord->setEchoMode(QLineEdit::Password);
            return false;
        }
        return false;
    }
    else if(watched == ui->lineEditCD)
    {
        if(event->type() == QEvent::FocusIn)
        {
            m_bgetFocus = true;
            return false;
        }
        else
        {
            m_bgetFocus = false;
            return false;
        }
    }

    return Widget::eventFilter(watched, event);
}


void Widget::showmessage(QString msg)
{
    ui->textBrowser->append(msg);
}



void Widget::doneSlot(bool err)
{
    if(err)
    {
        ui->textBrowser->append("ftp->error: "+ ftp->errorString());
    }
}

void Widget::ftpCommandFinished(int,bool error)
{
    QFtp::Command command = ftp->currentCommand();
    ui->textBrowser->append(QString::fromLocal8Bit("%1-%2-%3").arg(m_cmdMap.value(command)).arg(error).arg(ftp->errorString()));
    isError = error;

    if(command == QFtp::Get)
    {
        if(!error)
        {
            ui->textBrowser->append("trans ok!");
        }
        clearProgresssBar();
    }
    else if(command == QFtp::Put)
    {
        if(!error)
        {
            ui->textBrowser->append("trans ok!");
            list();
        }
        clearProgresssBar();
    }
    else if(command == QFtp::Login)
    {
        if(!error)
        {
            m_bconnect = true;
        }
        else
        {
            m_bconnect = false;
        }
    }
    else if(command == QFtp::Mkdir)
    {
        if(!error)
        {
            list();
        }
    }
}

void Widget::commandStartedSlot(int cmd)
{
//    QFtp::Command command = ftp->currentCommand();
//    switch (command)
//    {
//    case QFtp::List:
//    {
//        break;
//    }
//    default:
//        break;
//    }
}

void Widget::stateChangedSlot(int state)
{
    switch (state) {
    case QFtp::Unconnected: {
        ui->textBrowser->append(QString::fromLocal8Bit("没有连接到主机"));
        break;
    }
    case QFtp::HostLookup: {
        ui->textBrowser->append(QString::fromLocal8Bit("正在进行主机名查找"));
        break;
    }
    case QFtp::Connecting: {
        ui->textBrowser->append(QString::fromLocal8Bit("正在尝试连接到主机"));
        break;
    }
    case QFtp::Connected: {
        m_bconnect = true;
        ui->textBrowser->append(QString::fromLocal8Bit("已实现与主机的连接"));
        break;
    }
    case QFtp::LoggedIn: {
        ui->textBrowser->append(QString::fromLocal8Bit("已实现连接和用户登录"));
        break;
    }
    case QFtp::Closing: {

        m_bconnect = false;
        ui->textBrowser->append(QString::fromLocal8Bit("连接正在关闭"));
        break;
    }
    default:
        break;
    }
}

void Widget::readyReadSlot()
{
    QByteArray ba = ftp->readAll();

    ui->textBrowser->append(QString::fromLocal8Bit("readyReadSlot:%1").arg(QString(ba)));
    QFile *file = new QFile(QDir::currentPath() + "/" + sCurFile);
    if (!file->open(QIODevice::WriteOnly))
    {
        ui->textBrowser->append(file->errorString());
        return;
    }


    if(file->write(ba,ba.size()) > 0)
    {
        ui->textBrowser->append(QString::fromLocal8Bit("文件保存到 %1").arg(file->fileName()));
    }
    file->flush();
    file->close();

    clearProgresssBar();
    sCurFile.clear();
}

void Widget::dataTransferProgressSlot(qint64 Transbyte, qint64 totalbyte)
{
    qint32 fileSize = (qint32)(Transbyte / (1024 * 1024));
    if(Transbyte > 0 && fileSize == 0)
    {
        fileSize = 1;
    }

    qint32 totalSize = (qint32)(totalbyte / (1024 * 1024));
    if(totalbyte > 0 && totalSize == 0)
    {
        totalSize = 1;
    }

    ui->progressBarUpload->setMaximum(totalSize);
    ui->progressBarUpload->setValue(fileSize);
}

void Widget::listInfoSlot(const QUrlInfo &urlInfo)
{
    addFileToWidget(urlInfo);
//    if(level == 1)
//    {
//        enumFile(urlInfo,ui->treeWidget);
//    }
//    else
//    {
//        if(m_lastTreeWidget != nullptr)
//        {
//            enumFile(urlInfo,m_lastTreeWidget);
//        }
//    }
}

void Widget::on_pushButtonDownload_clicked()
{
    if(!m_bconnect)
    {
        ui->textBrowser->append(QString::fromLocal8Bit("连接失败,不允许操作"));
        return;
    }
    if(sCurFile.isEmpty())
    {
        ui->textBrowser->append(QString::fromLocal8Bit("请先选择下载的文件"));
        return;
    }
    ftp->get(sCurPath + "/" + sCurFile);
}

int Widget::init()
{
    if(ui->lineEditHostAddress->text().isEmpty() || ui->lineEditPort->text().isEmpty())
    {
        ui->textBrowser->append("please input ip & port");
        return -1;
    }
    ftp->setTransferMode(QFtp::Passive);
    ftp->connectToHost(ui->lineEditHostAddress->text(), ui->lineEditPort->text().toInt());

    if(ui->lineEditUsrName->text().isEmpty() || ui->lineEditPassWord->text().isEmpty())
    {
        ui->textBrowser->append("please input user & pwd");
        return -1;
    }

    return ftp->login(ui->lineEditUsrName->text(), ui->lineEditPassWord->text());
}



void Widget::enumFile(const QUrlInfo &urlInfo,QTreeWidget *treewidget)
{
//    bool hasDir = false;
//    if(urlInfo.isDir())
//    {
//        ++level;
//        hasDir = true;
//        m_bhasDir = true;
//        QTreeWidgetItem *group = new QTreeWidgetItem(treewidget);
//        group->setText(0,urlInfo.name());
//        group->setFlags(Flags);
//        group->setCheckState(0, Qt::Unchecked);

//        m_lastTreeWidget = group;
////        ftp->list(urlInfo.name());
//    }
//    else
//    {
//        QTreeWidgetItem *group1 = nullptr;
//        if(level == 1)
//        {
//            QTreeWidgetItemIterator it(treewidget);
//            bool isExists = false;
//            while(*it)
//            {
//                if((*it)->text(0) == "./")
//                {
//                    isExists = true;
//                    group1 = (*it);
//                    break;
//                }
//                ++it;
//            }
//            if(!isExists)
//            {
//                group1 = new QTreeWidgetItem(treewidget);
//                group1->setText(0,"./");
//                group1->setFlags(Flags);
//                group1->setCheckState(0, Qt::Unchecked);
//            }
//        }

//        if(group1 != nullptr)
//        {
//            QTreeWidgetItem *group = new QTreeWidgetItem(group1);
//            group->setText(0,urlInfo.name());
//            group->setFlags(Flags);
//            group->setCheckState(0, Qt::Unchecked);
//        }
//        else
//        {
//            QTreeWidgetItem *group = new QTreeWidgetItem(treewidget);
//            group->setText(0,urlInfo.name());
//            group->setFlags(Flags);
//            group->setCheckState(0, Qt::Unchecked);
//        }

//    }
//    if(hasDir)
//    {
//        --level;
//    }
}

void Widget::enumFile(const QUrlInfo &urlInfo, QTreeWidgetItem *treewidgetItem)
{
//    bool hasDir = false;
//    if(urlInfo.isDir())
//    {
//        ++level;
//        hasDir = true;

//        m_bhasDir = true;
//        QTreeWidgetItem *group = new QTreeWidgetItem(treewidgetItem);
//        group->setText(0,urlInfo.name());
//        group->setFlags(Flags);
//        group->setCheckState(0, Qt::Unchecked);

//        m_lastTreeWidget = group;
//        ftp->list(urlInfo.name());
//    }
//    else
//    {
//        QTreeWidgetItem *group1 = nullptr;
//        if(level == 1)
//        {
//            QTreeWidgetItemIterator it(treewidgetItem);
//            bool isExists = false;
//            while(*it)
//            {
//                if((*it)->text(0) == "./")
//                {
//                    isExists = true;
//                    group1 = (*it);
//                    break;
//                }
//                ++it;
//            }
//            if(!isExists)
//            {
//                group1 = new QTreeWidgetItem(treewidgetItem);
//                group1->setText(0,"./");
//                group1->setFlags(Flags);
//                group1->setCheckState(0, Qt::Unchecked);
//            }
//        }

//        if(group1 != nullptr)
//        {
//            QTreeWidgetItem *group = new QTreeWidgetItem(group1);
//            group->setText(0,urlInfo.name());
//            group->setFlags(Flags);
//            group->setCheckState(0, Qt::Unchecked);
//        }
//        else
//        {
//            QTreeWidgetItem *group = new QTreeWidgetItem(treewidgetItem);
//            group->setText(0,urlInfo.name());
//            group->setFlags(Flags);
//            group->setCheckState(0, Qt::Unchecked);
//        }

//    }

//    if(hasDir)
//    {
//        --level;
//    }
}

void Widget::addFileToWidget(const QUrlInfo &urlInfo)
{
   int row = ui->tableWidget->rowCount();
   ui->tableWidget->insertRow(row);


   if(urlInfo.isDir())
   {
        QPushButton *btn = new QPushButton;
        btn->setStyleSheet("border:none;color:transparent");
        ui->tableWidget->setCellWidget(row,0,(QWidget *)btn);
        btn->setText("2");
        btn->setIcon(QIcon(":/image/dir.png"));
   }
   else
   {
       QPushButton *btn = new QPushButton;
       btn->setStyleSheet("border:none;color:transparent");
       ui->tableWidget->setCellWidget(row,0,(QWidget *)btn);
       btn->setText("1");
       btn->setIcon(QIcon(":/image/file.png"));
   }
   ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
   ui->tableWidget->setItem(row, 1, new QTableWidgetItem(urlInfo.name()));
   ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(urlInfo.size())));
}

void Widget::cd(const QString &sPath)
{
    QString slastPath = sCurPath;
    if(sPath == "../")
    {
        if(sCurPath.split("/").size() <= 2)
        {
            sCurPath = "/";
        }
        else
        {
            sCurPath = sCurPath.remove(sCurPath.lastIndexOf("/"),100);
        }
    }
    else
    {
        ui->textBrowser->append(sPath +" spath");
        if(!(sPath.split("/").size() > 2 || sPath == sCurPath))
        {
            sCurPath = sCurPath.append("/" + sPath);
        }
    }

    if(sCurPath.startsWith("//"))
    {
        sCurPath.remove(0,1);
    }
    ui->lineEditCD->setText(sCurPath);

    int row = ui->tableWidget->rowCount();
    for(int i = row - 1;i>=1;--i)
    {
        ui->tableWidget->removeRow(i);
    }
    ftp->cd(sPath);
    if(isError)
    {
        sCurPath = slastPath;
        ui->lineEditCD->setText(sCurPath);
    }
    ftp->list();
}

void Widget::list()
{
    int row = ui->tableWidget->rowCount();
    for(int i = row - 1;i>=1;--i)
    {
        ui->tableWidget->removeRow(i);
    }
    ftp->list();
}

void Widget::clearProgresssBar()
{
    QTimer::singleShot(300, [&]()
    {
        ui->progressBarUpload->setMaximum(100);
        ui->progressBarUpload->setValue(0);
    });

}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        sCurPath = ui->lineEditCD->text();
        if(!sCurPath.isEmpty())
        {
            cd(sCurPath);
        }
    }
}

//拖放
void Widget::dragEnterEvent(QDragEnterEvent *event)
{
    ui->textBrowser->append("dragEnterEvent");
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void Widget::dragLeaveEvent(QDragLeaveEvent *event)
{
    ui->textBrowser->append("dragLeaveEvent");
}
void Widget::dropEvent(QDropEvent *event)
{
    ui->textBrowser->append("dropEvent");
    const QMimeData *mimedata = event->mimeData();
    if(mimedata->hasUrls())
    {
        QList<QUrl>urlList = mimedata->urls();
        if(urlList.isEmpty())
        {
            return ;
        }
        QString fileName = urlList.at(0).toLocalFile();
        sTransDirName = fileName;
        ui->textBrowser->append("dragfileName:"+fileName);
        if(fileName.isEmpty())
        {
            return;
        }
        QFileInfo fileInfo(fileName);
        if(fileInfo.isFile())
        {
            QFile file(fileName);
            if(!file.open(QIODevice::ReadOnly))
            {
                return;
            }
            if(m_bconnect)
            {
                ftp->put(file.readAll(),ui->lineEditCD->text() + "/" + file.fileName().split("/").last());
            }
        }
        else if(fileInfo.isDir())
        {
            if(!m_bconnect)
            {
                return;
            }
//            ftp->rmdir(fileInfo.fileName());
            ui->textBrowser->append(fileInfo.filePath());
            ftp->mkdir(fileInfo.fileName());

            QTimer::singleShot(1000, [&]()
            {
                if(isError)
                {
                    ui->textBrowser->append("mkdir error");
                    return;
                }
                ui->textBrowser->append("mkdir ok");
                enumfiles();
            });
        }
    }

}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget *w = this->focusWidget();
    if(m_pStartPoint.isNull())
    {
        m_pStartPoint = w->pos();
    }

    w->move(event->pos().x(),event->pos().y());

    if(event->pos().x() > this->width() || event->pos().y() > this->height()
            || event->pos().x() < 0 || event->pos().y() < 0)
    {
        w->move(m_pStartPoint);
        m_pStartPoint = m_pEmptyPoint;
//        m_pStartPoint = event->pos();
    }

//    if(w->inherits("QPushButton"))
//    {
//        QPushButton *btn = qobject_cast<QPushButton *>(w);
//        if(!btn->objectName().isEmpty())
//        {
//            btn->move(event->pos().x(),event->pos().y());
//        }
//    }
//    QWidget::mouseMoveEvent(event);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    QWidget *w = this->focusWidget();
    if(m_pStartPoint.isNull())
    {
        m_pStartPoint = w->pos();
    }

    if(event->button() == Qt::LeftButton)
    {
        ui->textBrowser->append("mousleftepress");
        m_pStartPoint = event->pos();
    }
    m_bpressflag = true;
    QWidget::mousePressEvent(event);
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    m_bpressflag = false;
    QWidget::mouseReleaseEvent(event);
}

void Widget::on_pushButtonReconn_clicked()
{
    QTimer::singleShot(3000, [&]()
    {
        if(!m_bconnect)
        {
            ui->textBrowser->append("connect fail");
            ftp->close();
        }
    });
    sCurPath = ui->lineEditCD->text();
    if(init() != QFtp::State::Connected)
    {
        ui->textBrowser->append(QString::fromLocal8Bit("连接失败"));
    }
    else
    {
        ftp->cd(sCurPath);
        ftp->list();
    }
}


void Widget::on_tableWidget_itemDoubleClicked(QTableWidgetItem *item)
{
    if(!m_bconnect)
    {
        ui->textBrowser->append(QString::fromLocal8Bit("连接失败,不允许操作"));
        return;
    }
    ui->textBrowser->append(QString::number(item->row()));
    if(item->row() == 0)
    {
        cd("../");
    }
    else
    {
        QString sPath = ui->tableWidget->item(item->row(),1)->text();
        ui->textBrowser->append(sPath);

        QPushButton *btn = (QPushButton*)ui->tableWidget->cellWidget(item->row(),0);
        if(btn->text() == "1")
        {
            sCurFile = sPath;
        }
        else
        {
            cd(sPath);
        }
    }
}

void Widget::on_pushButtonUpload_clicked()
{
    if(!m_bconnect)
    {
        ui->textBrowser->append(QString::fromLocal8Bit("连接失败,不允许操作"));
        return;
    }
    QString upLoadFile = QFileDialog::getOpenFileName(this,
                                                      QString::fromLocal8Bit("选择需要上传的文件"));

    if(upLoadFile.isEmpty())
    {
        return;
    }

    QFile mfile(upLoadFile);
    if(!mfile.open(QIODevice::ReadOnly))
    {
        ui->textBrowser->append(mfile.errorString());
        return;
    }
    ftp->put(mfile.readAll(),ui->lineEditCD->text() + "/" + mfile.fileName().split("/").last());
}

void Widget::on_pushButtonClose_clicked()
{
    ftp->close();

    int row = ui->tableWidget->rowCount();
    for(int i = row - 1;i>=1;--i)
    {
        ui->tableWidget->removeRow(i);
    }
    ui->lineEditCD->setText("/");
    clearProgresssBar();
    m_bconnect = false;
}
void Widget::enumfiles(QString sFilePath)
{
    QDir dir(sTransDirName);
    if(dir.exists())
    {
        foreach(QFileInfo mitem,dir.entryInfoList())
        {
            if(mitem.isFile())
            {
                QFile mfile(mitem.filePath());
                if(!mfile.open(QIODevice::ReadOnly))
                {
                    ui->textBrowser->append(mitem.fileName() + " " + mfile.errorString());
                    return;
                }
                ui->textBrowser->append(ui->lineEditCD->text() + "/" + sTransDirName.split("/").last() + "/" + mitem.fileName());
                ftp->put(mfile.readAll(),ui->lineEditCD->text() + "/" + sTransDirName.split("/").last() + "/" + mitem.fileName());
            }
            else
            {
                //
            }
        }
    }
    else
    {
        ui->textBrowser->append(sTransDirName + "is not exist");
    }
}

QString Widget::CurIP()
{
    QString sIp = "";
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
        if(address.isNull())
            continue;
        QAbstractSocket::NetworkLayerProtocol protocol = address.protocol();
        if(protocol != QAbstractSocket::IPv4Protocol)
            continue;
        ui->textBrowser->append(address.toString());
    }
    return sIp;
}

void Widget::on_pushButton_mkdir_clicked()
{
    if(ui->lineEdit_mkdir->text().isEmpty())
    {
        return;
    }
    ftp->mkdir(ui->lineEdit_mkdir->text());
}
