#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QObject>
#include <QFtp>
#include <QMap>
#include <QTreeWidget>
#include <QTableWidgetItem>
#include <QEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QDrag>
#include <QMimeData>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();



    virtual bool eventFilter(QObject *watched, QEvent *event);
    void showmessage(QString);
private slots:

    void doneSlot(bool);

    void ftpCommandFinished(int,bool);

    void commandStartedSlot(int);

    void stateChangedSlot(int);

    void readyReadSlot();

    void dataTransferProgressSlot(qint64,qint64);

    void listInfoSlot(const QUrlInfo &);

    void on_pushButtonDownload_clicked();

    void on_pushButtonReconn_clicked();

    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);

    void on_pushButtonUpload_clicked();

    void on_pushButtonClose_clicked();

    void on_pushButton_mkdir_clicked();

private:
    int init();
    void enumFile(const QUrlInfo &urlInfo,QTreeWidget *treewidget);

    void enumFile(const QUrlInfo &urlInfo,QTreeWidgetItem *treewidgetItem);


    void addFileToWidget(const QUrlInfo &urlInfo);


    void cd(const QString &sPath);

    void list();
    void clearProgresssBar();

    void enumfiles(QString sFilePath = "");


    QString CurIP();
protected:

    virtual void keyPressEvent(QKeyEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent *event);

    virtual void dragLeaveEvent(QDragLeaveEvent *event);

    virtual void dropEvent(QDropEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);

    virtual void mouseReleaseEvent(QMouseEvent *event);
private:
    Ui::Widget *ui;

    QFtp *ftp;

    QMap<int,QString> m_cmdMap;

    QTreeWidgetItem *m_lastTreeWidget = nullptr;

    bool m_bhasDir = false;

    int isOver = 0;

    QString sCurPath = "";
    QString sCurFile = "";
    QString sTransDirName = "";

    bool isError = false;
    bool m_bconnect = false;
    bool m_bgetFocus = true;

    bool m_bpressflag = false;

    QPoint m_pStartPoint;
    QPoint m_pEmptyPoint;
};
#endif // WIDGET_H
