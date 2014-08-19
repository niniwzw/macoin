// Copyright (c) 2011-2014 The Macoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logindialog.h"
#include "ui_logindialog.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "guiutil.h"
//#include "receiverequestdialog.h"
#include "addresstablemodel.h"
//#include "recentrequeststablemodel.h"
#include "init.h"

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QTextDocument>
#include <QScrollBar>

Value CallRPC(string args)
{
    vector<string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    Array params = RPCConvertValues(strMethod, vArgs);

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        Value result = (*method)(params, false);
        return result;
    }
    catch (Object& objError)
    {
        throw runtime_error(find_value(objError, "message").get_str());
    }
}

void LoginThread::setpassword(QString password){
	strpassword = password ;
}
void LoginThread::setusername(QString name){
	strusername = name ;
}

void LoginThread::setgetinfoobj(Object obj){
	m_obj = obj ;
}

void LoginThread::startwork()  
{  
	start(); //HighestPriority 
}  

int LoginThread::Login()
{
		try{
			OAuth2::login(strusername.toStdString() ,strpassword.toStdString());
		}catch(...){
			return 1;
		}
		if (OAuth2::getAccessToken() == "")
		{
			return 2;
		}
		//重置服务器端的私钥信息，信息通过user/info设置
		pwalletMain->DeleteServerKey();
		pwalletMain->DeleteRealNameAddress();
		return 0 ;
}

static bool CheckAddress(string publocal, string pubserver, string address) {
	const Object addrinfo = CallRPC(string("validatepubkey ") + publocal).get_obj();
	const bool ismine = find_value(addrinfo, "ismine").get_bool();
	if (!ismine)
	{
		return false;
	}
	CallRPC(string("setserverpubkey ") + pubserver);
	pwalletMain->AddRealNameAddress(address);
	return true;
}

int LoginThread::SubScribeAddress()
{
		Value addrvalue = CallRPC("getnewpubkey2");
		string pubkey = addrvalue.get_str();
        string password = "1";
		const Object addrinfo = CallRPC(string("validatepubkey ") + pubkey).get_obj();
		const string addr = find_value(addrinfo, "address").get_str();
        
		//解锁钱包
		if (pwalletMain->IsLocked())
		{
			CallRPC(string("walletpassphrase ")+password+" 30");
		}
		Value key      = CallRPC(string("dumpprivkey ") + addr);
		string privkey = key.get_str();
		
		//生成salt
        uint256 hash1 = Hash(privkey.begin(), privkey.end());
		Object multiinfo ;
		try{
			multiinfo = Macoin::addmultisigaddress(pubkey, hash1.GetHex());
		}catch(...){
              return 11;
		}
		if (find_value(multiinfo,  "error").type() != null_type) {
              return 12;
        }
		const string pubkey1 = "\"" + find_value(multiinfo, "pubkey1").get_str() + "\"";
		const string pubkey2 = "\"" + find_value(multiinfo, "pubkey2").get_str() + "\"";
		const string multiaddr = find_value(multiinfo, "addr").get_str();
		const Value multisigwallet = CallRPC(string("addmultisigaddress 2 ") + "["+pubkey1+","+pubkey2+"]" + " Real");
		if(multisigwallet.type()  != str_type){
			return 13;
		}
		return 14 ;
}

void LoginThread::run()  
{
	int ret =-1;
	switch(m_type){
		case 1:
		{
			ret = Login();
			emit notify(ret);  
			break;
		}
		case 2:
		{
		   map<string, string> params;
		   Object userinfo = Macoin::api("user/info", params, "GET");
			emit getinfonotify(userinfo);  
			break;
		}
		case 3:
		{
			ret = SubScribeAddress();
			emit notify(ret);  
			break;
		}
	}
}  



LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    model(0)
{
    ui->setupUi(this);

	ui->passwordLabel->setEchoMode (QLineEdit::Password);
	ui->label_Link->setText(tr("<a href = 'https://zc.macoin.org/user/findPass'>forget password</a>"));
	ui->label_Link->setOpenExternalLinks( true );

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    //ui->clearButton->setIcon(QIcon());
    ui->loginButton->setIcon(QIcon());
    //ui->showRequestButton->setIcon(QIcon());
    //ui->removeRequestButton->setIcon(QIcon());
#endif

    // context menu actions
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    //QAction *copyMessageAction = new QAction(tr("Copy message"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyLabelAction);
    //contextMenu->addAction(copyMessageAction);
    contextMenu->addAction(copyAmountAction);

    // context menu signals
    //connect(ui->recentRequestsView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    //connect(copyMessageAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
	
	connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(on_loginButton_clicked()));
	connect(ui->SubscriptButton, SIGNAL(clicked()), this, SLOT(on_subscriptButton_clicked()));
	connect(ui->LogoutButton, SIGNAL(clicked()), this, SLOT(on_logoutButton_clicked()));


	ui->frame->setVisible(false);
		

    //connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
}

void LoginDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        //connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        //updateDisplayUnit();

        //ui->recentRequestsView->setModel(model->getRecentRequestsTableModel());
        //ui->recentRequestsView->setAlternatingRowColors(true);
        //ui->recentRequestsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        //ui->recentRequestsView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Date, 130);
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Label, 120);
#if QT_VERSION < 0x050000
        //ui->recentRequestsView->horizontalHeader()->setResizeMode(RecentRequestsTableModel::Message, QHeaderView::Stretch);
#else
        //ui->recentRequestsView->horizontalHeader()->setSectionResizeMode(RecentRequestsTableModel::Message, QHeaderView::Stretch);
#endif
        //ui->recentRequestsView->horizontalHeader()->resizeSection(RecentRequestsTableModel::Amount, 100);

        //model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
		connect(model, SIGNAL(displayLoginView()), this, SLOT(showLoginView()));
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::clear()
{
    //ui->passwordLabel->clear();
    //ui->usernameLabel->setText("");
    //ui->reqMessage->setText("");
    //ui->reuseAddress->setChecked(false);
    //updateDisplayUnit();
}

void LoginDialog::reject()
{
    clear();
}

void LoginDialog::accept()
{
    clear();
}

void LoginDialog::updateDisplayUnit()
{
    /*if(model && model->getOptionsModel())
    {
        ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }*/
}


void LoginDialog::on_logoutButton_clicked()
{
    ui->label_Address1->setText("");
	ui->label_Address2->setText("");
	ui->label_Address3->setText("");

	ui->UIDLabel->setText("");
	ui->phoneLabel->setText("");
	ui->emailLabel->setText("");

	ui->statusLabel->setText("");

	ui->passwordLabel->setText("");
	ui->frame2->setVisible(true);
	ui->LogoutButton->setVisible(true);
	OAuth2::clear();
	ui->frame->setVisible(false);
	pwalletMain->DeleteServerKey();
    pwalletMain->DeleteRealNameAddress();
	CallRPC("rescanwallet");
}

void LoginDialog::on_subscriptButton_clicked()
{
	if(OAuth2::getAccessToken() != ""){

		render2 = new LoginThread(3);
		connect(render2,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
		ui->SubscriptButton->setEnabled(false);
		render2->startwork(); 	

	}else{
		ui->passwordLabel->setText("");
		ui->frame2->setVisible(true);
		ui->frame->setVisible(false);
		  QMessageBox::warning(this, "macoin",
                (tr("please login first!")),
                QMessageBox::Ok, QMessageBox::Ok);
	}
    //BOOST_CHECK(multiaddr == multisigwallet);		
}

void LoginDialog::showLoginView()
{
		ui->frame->setVisible(false);
		ui->passwordLabel->setText("");
		ui->frame2->setVisible(true);
		ui->LogoutButton->setVisible(false);
}

void LoginDialog::getUserInfo()
{
   map<string, string> params;
   const Object userinfo = Macoin::api("user/info", params, "GET");
   
   const string UID = find_value(userinfo, "id").get_str() ;
   const string phone = find_value(userinfo, "mobile").get_str() ;
   const string email = find_value(userinfo, "email").get_str() ;
   const string ifverify = find_value(userinfo, "ifverify").get_str() ;
   if (ifverify == "1")
   {
		ui->statusLabel->setText((tr("pass realname verify")));
   }else{
	    ui->statusLabel->setText((tr("no pass realname verify")));
   }
   Value objaddresses = find_value(userinfo, "address") ;
   if (objaddresses.type() == array_type)
   {
	   Array addrArray = objaddresses.get_array();
	   int size = addrArray.size();
	   for(int i = 0 ;i<size ;i++){
		   Value addvalue = addrArray[i];
		   if (addvalue.type() == obj_type)
		   {   
			   string publocal = find_value(addvalue.get_obj(), "publocal").get_str();
			   string pubserver = find_value(addvalue.get_obj(), "pubserver").get_str();
			   string serveraddr = find_value(addvalue.get_obj(), "addr").get_str();
			   if (!::CheckAddress(publocal, pubserver, serveraddr))
			   {
					QMessageBox::warning(this, "macoin",
							tr("local private key is missing, logout"),
							QMessageBox::Ok, QMessageBox::Ok);
					on_logoutButton_clicked();
					return ;
			   }
			   if (i==0)
			   {
				   ui->label_Address1->setText(QString::fromStdString(serveraddr));
			   }else if(i==1){
				   ui->label_Address2->setText(QString::fromStdString(serveraddr));
			   }else if(i==2){
				   ui->label_Address3->setText(QString::fromStdString(serveraddr));
			   }
		   }
	   }
	   CallRPC("rescanwallet");
   }

	ui->UIDLabel->setText(QString::fromStdString(UID));
	ui->phoneLabel->setText(QString::fromStdString(phone));
	ui->emailLabel->setText(QString::fromStdString(email));

}
/*
void LoginDialog::SubScribeAddress()
{
		Value addrvalue = CallRPC("getnewaddress");
		//BOOST_CHECK(addrvalue.type() == str_type);
		string addr = addrvalue.get_str();

		const Object addrinfo = CallRPC(string("validateaddress ") + addr).get_obj();
		const string pubkey = find_value(addrinfo, "pubkey").get_str();
		Object multiinfo ;
		try{
			multiinfo = Macoin::addmultisigaddress(pubkey);
		}catch(...){
			  QMessageBox::warning(this, "macoin",
					(tr("please login first or checking network!")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
              return;
		}
		if (find_value(multiinfo,  "error").type() != null_type) {
  			  QMessageBox::warning(this, "macoin",
					(tr("no more than three address or not login")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
              return;
        } 
		const string pubkey1 = "\"" + find_value(multiinfo, "pubkey1").get_str() + "\"";
		const string pubkey2 = "\"" + find_value(multiinfo, "pubkey2").get_str() + "\"";
		const string multiaddr = find_value(multiinfo, "addr").get_str();
		const Value multisigwallet = CallRPC(string("addmultisigaddress 2 ") + "["+pubkey1+","+pubkey2+"]" + " macoin_validate_wallet");
		if(multisigwallet.type()  != str_type){
  			  QMessageBox::warning(this, "macoin",
					(tr("add multisigaddress error ")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			return ;
		}
		getUserInfo();
  		QMessageBox::warning(this, "macoin",
					(tr("apply Success!")),
					QMessageBox::Ok, QMessageBox::Ok);
		ui->SubscriptButton->setEnabled(true);
}


void LoginDialog::Login()
{
			QString strusername = ui->usernameLabel->text();
			QString strpassword = ui->passwordLabel->text();
			try{
				OAuth2::login(strusername.toStdString() ,strpassword.toStdString());
				
			}catch(...){
				QMessageBox::warning(this, "macoin",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->loginButton->setEnabled(true);
				return ;
			}
		   if (OAuth2::getAccessToken() == "")
		   {
				QMessageBox::warning(this, "macoin",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->loginButton->setEnabled(true);
				return ;
		   }


		   ui->frame2->setVisible(false);
		   ui->LogoutButton->setVisible(true);
		   ui->frame->setVisible(true);
		   ui->loginButton->setEnabled(true);

			render1 = new LoginThread(2);
			 connect(render1,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
			 render1->startwork(); 	
}
*/
void LoginDialog::OnNotify(int  type)  
{  
	switch (type)
	{
		case 0:
		{
		   ui->frame2->setVisible(false);
		   ui->LogoutButton->setVisible(true);
		   ui->frame->setVisible(true);
		   ui->loginButton->setEnabled(true);

		   getUserInfo();	
		   break;
		}
		case 1:
		{
				QMessageBox::warning(this, "macoin",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->loginButton->setEnabled(true);
			 //Login();
			 break;
		}
		case 2:
		{
			//getUserInfo();
				QMessageBox::warning(this, "macoin",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->loginButton->setEnabled(true);
			break;
		}
		case 11:
		{
			  QMessageBox::warning(this, "macoin",
					(tr("please login first or checking network!")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			break;
		}
		case 12:
		{
  			  QMessageBox::warning(this, "macoin",
					(tr("no more than three address or not login")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			break;
		}
		case 13:
		{
  			  QMessageBox::warning(this, "macoin",
					(tr("add multisigaddress error ")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			  break;
		}
		case 14:
		{
		   //render1 = new LoginThread(2);
		   //connect(render1,SIGNAL(getinfonotify(Object)),this,SLOT(OnNotifyGetInfo(Object)));  
		   //render1->startwork(); 
		   getUserInfo();	
			QMessageBox::warning(this, "macoin",
						(tr("apply Success!")),
						QMessageBox::Ok, QMessageBox::Ok);
			ui->SubscriptButton->setEnabled(true);
			break;
		}
	}

}

void LoginDialog::OnNotifyGetInfo(Object userinfo)
{
   const string UID = find_value(userinfo, "id").get_str() ;
   const string phone = find_value(userinfo, "mobile").get_str() ;
   const string email = find_value(userinfo, "email").get_str() ;
   const string ifverify = find_value(userinfo, "ifverify").get_str() ;
   if (ifverify == "1")
   {
		ui->statusLabel->setText((tr("pass realname verify")));
   }else{
	    ui->statusLabel->setText((tr("no pass realname verify")));
   }
   Value objaddresses = find_value(userinfo, "address") ;
   if (objaddresses.type() == array_type)
   {
	   Array addrArray = objaddresses.get_array();
	   int size = addrArray.size();
	   for(int i = 0 ;i<size ;i++){
		   Value addvalue = addrArray[i];
		   if (addvalue.type() == obj_type)
		   {   
			   string publocal = find_value(addvalue.get_obj(), "publocal").get_str();
			   string pubserver = find_value(addvalue.get_obj(), "pubserver").get_str();
			   string serveraddr = find_value(addvalue.get_obj(), "addr").get_str();
			   if (!::CheckAddress(publocal, pubserver, serveraddr))
			   {
					QMessageBox::warning(this, "macoin",
							tr("local private key is missing, logout"),
							QMessageBox::Ok, QMessageBox::Ok);
					on_logoutButton_clicked();
					return ;
			   }
			   if (i==0)
			   {
				   ui->label_Address1->setText(QString::fromStdString(serveraddr));
			   }else if(i==1){
				   ui->label_Address2->setText(QString::fromStdString(serveraddr));
			   }else if(i==2){
				   ui->label_Address3->setText(QString::fromStdString(serveraddr));
			   }
		   }
	   }
   }

	ui->UIDLabel->setText(QString::fromStdString(UID));
	ui->phoneLabel->setText(QString::fromStdString(phone));
	ui->emailLabel->setText(QString::fromStdString(email));
}

void LoginDialog::on_loginButton_clicked()
{
    if(!model || !model->getOptionsModel() || !model->getAddressTableModel() )//|| !model->getRecentRequestsTableModel()
        return;

    QString strusername = ui->usernameLabel->text();
	QString strpassword = ui->passwordLabel->text();
	if (strusername == "" || strpassword == "")
	{
        QMessageBox::warning(this, "macoin",
                tr("username or password is empty"),
                QMessageBox::Ok, QMessageBox::Ok);
		return ;
	}
	
	render = new LoginThread(1);
     connect(render,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
	 ui->loginButton->setEnabled(false);
	 render->setpassword(strpassword);
	 render->setusername(strusername);
     render->startwork();    

}

void LoginDialog::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
   /* const RecentRequestsTableModel *submodel = model->getRecentRequestsTableModel();
    ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(submodel->entry(index.row()).recipient);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
	*/
}

void LoginDialog::on_showRequestButton_clicked()
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();

    //foreach (QModelIndex index, selection)
    //{
        //on_recentRequestsView_doubleClicked(index);
    //}
}

void LoginDialog::on_removeRequestButton_clicked()
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    // correct for selection mode ContiguousSelection
    //QModelIndex firstIndex = selection.at(0);
    //model->getRecentRequestsTableModel()->removeRows(firstIndex.row(), selection.length(), firstIndex.parent());
}

void LoginDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        // press return -> submit form
        if (ui->usernameLabel->hasFocus() || ui->passwordLabel->hasFocus() )
        {
            event->ignore();
            on_loginButton_clicked();
            return;
        }
    }

    this->QDialog::keyPressEvent(event);
}

// copy column of selected row to clipboard
void LoginDialog::copyColumnToClipboard(int column)
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    // correct for selection mode ContiguousSelection
//    QModelIndex firstIndex = selection.at(0);
    //GUIUtil::setClipboard(model->getRecentRequestsTableModel()->data(firstIndex.child(firstIndex.row(), column), Qt::EditRole).toString());
}

// context menu
void LoginDialog::showMenu(const QPoint &point)
{
    //if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        //return;
    //QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    //if(selection.empty())
        //return;
    //contextMenu->exec(QCursor::pos());
}

// context menu action: copy label
void LoginDialog::copyLabel()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Label);
}

// context menu action: copy message
void LoginDialog::copyMessage()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Message);
}

// context menu action: copy amount
void LoginDialog::copyAmount()
{
    //copyColumnToClipboard(RecentRequestsTableModel::Amount);
}
