// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>
#include <QThread>  


//#include "rpcserver.h"
//#include "rpcclient.h"
#include "bitcoinrpc.h"

#include "base58.h"

#include <boost/algorithm/string.hpp>



using namespace std;
using namespace json_spirit;



namespace Ui {
    class LoginDialog;
}
class WalletModel;
class OptionsModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE


class LoginThread : public QThread  
{  
        Q_OBJECT  
signals:  
        void notify(int);  
		void getinfonotify(Object);

private:
	int  m_type ;
	QString strusername ;
	QString strpassword;
	Object  m_obj ;
public:  
    LoginThread(int type)
    {  
			m_type = type ;
    };  

	void setpassword(QString password);
	void setusername(QString name);
	void setgetinfoobj(Object obj);
	void startwork()  ;
	int  Login();
	int SubScribeAddress();
protected:
	void run() ;
};   



/** Dialog for requesting payment of bitcoins */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

    void setModel(WalletModel *model);

public slots:
    void clear();
    void reject();
    void accept();

protected:
    virtual void keyPressEvent(QKeyEvent *event);


private:
    Ui::LoginDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;
    void copyColumnToClipboard(int column);
	LoginThread *render;  
	LoginThread *render1;  
	LoginThread *render2;  

	void SubScribeAddress();
	void Login();

private slots:
    void on_loginButton_clicked();
	void on_logoutButton_clicked();
	void on_subscriptButton_clicked();
    void on_showRequestButton_clicked();
    void on_removeRequestButton_clicked();
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);
    void updateDisplayUnit();
    void showMenu(const QPoint &);
    void copyLabel();
    void copyMessage();
    void copyAmount();

	void showLoginView();

	void getUserInfo();
	void OnNotify(int type) ;
	void OnNotifyGetInfo(Object userinfo);
signals:
    void displayLoginView();
};

#endif // LOGINDIALOG_H
