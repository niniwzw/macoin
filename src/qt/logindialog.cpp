// Copyright (c) 2011-2014 The bityuan developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logindialog.h"
#include "ui_logindialog.h"


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

extern bool fWalletUnlockStakingOnly;

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

void LoginThread::setUnlockContext(WalletModel::UnlockContext * ctx)
{
	m_ctx = ctx ;
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
			const Object obj = OAuth2::login(strusername.toStdString() ,strpassword.toStdString());
			Value errorvalue = find_value(obj , "error");
			if (errorvalue.type() != null_type)
			{
				Value codevalue = find_value(obj , "code");
				if(codevalue.type() == int_type ){
					return codevalue.get_int();
				}
				return 1;
			}

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
	try{
			Value addrvalue = CallRPC("getnewpubkey2");
			string pubkey = addrvalue.get_str();
			const Object addrinfo = CallRPC(string("validatepubkey ") + pubkey).get_obj();
			const string addr = find_value(addrinfo, "address").get_str();
			
			Value key      = CallRPC(string("dumpprivkey ") + addr);
			string privkey = key.get_str();
			
			//生成salt
			uint256 hash1 = Hash(privkey.begin(), privkey.end());
			Object multiinfo ;
			
				multiinfo = bityuan::addmultisigaddress(pubkey, hash1.GetHex());
				Value errorvalue = find_value(multiinfo , "error");
				if (errorvalue.type() != null_type)
				{
					Value codevalue = find_value(multiinfo , "code");
					if(codevalue.type() == int_type ){
						delete m_ctx;
						m_ctx = NULL;
						return codevalue.get_int();
					}
					delete m_ctx;
					m_ctx = NULL;
					return 11;
				}

			const string pubkey1 = "\"" + find_value(multiinfo, "pubkey1").get_str() + "\"";
			const string pubkey2 = "\"" + find_value(multiinfo, "pubkey2").get_str() + "\"";
			const string multiaddr = find_value(multiinfo, "addr").get_str();
			const Value multisigwallet = CallRPC(string("addmultisigaddress 2 ") + "["+pubkey1+","+pubkey2+"]" + " Real");
			if(multisigwallet.type()  != str_type){
				delete m_ctx;
				m_ctx = NULL;
				return 13;
			}
			delete m_ctx;
			m_ctx = NULL;
			return 14 ;
		}catch(...){
			if(m_ctx != NULL){
				delete m_ctx;
				m_ctx = NULL;
			}
             return 11;
		}
}

void LoginThread::run()  
{
	int ret =-1;
	switch(m_type){
		case 1:
		{
			ret = Login();
			emit notify(ret,1);  
			break;
		}
		case 2:
		{
		    map<string, string> params;
		    Object userinfo = bityuan::api("user/info", params, "GET");
			emit getinfonotify(userinfo);  
			break;
		}
		case 3:
		{
			ret = SubScribeAddress();
			emit notify(ret,3);  
			break;
		}
		case 4:
		{
			CallRPC("rescanwallet");
			emit notify(15,4);  
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
	ui->label_Link->setText(tr("<a href = 'https://zc.bityuan.org/user/findPass'>forget password</a>"));
	ui->label_Link->setOpenExternalLinks( true );

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    //ui->clearButton->setIcon(QIcon());
    ui->LoginButton->setIcon(QIcon());
    //ui->showRequestButton->setIcon(QIcon());
    //ui->removeRequestButton->setIcon(QIcon());
#endif
	connect(ui->LoginButton, SIGNAL(clicked()), this, SLOT(on_loginButton_clicked()));
	connect(ui->SubscriptButton, SIGNAL(clicked()), this, SLOT(on_subscriptButton_clicked()));
	connect(ui->LogoutButton, SIGNAL(clicked()), this, SLOT(on_logoutButton_clicked()));
	ui->frame->setVisible(false);
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
	//ui->label_Address2->setText("");
	ui->label_Address3->setText("");

	ui->UIDLabel->setText("");
	//ui->phoneLabel->setText("");
	ui->emailLabel->setText("");

	ui->statusLabel->setText("");

	ui->passwordLabel->setText("");
	ui->frame2->setVisible(true);
	ui->LogoutButton->setVisible(true);
	OAuth2::clear();
	ui->frame->setVisible(false);
	pwalletMain->DeleteServerKey();
    pwalletMain->DeleteRealNameAddress();

	render3 = new LoginThread(4);
	connect(render3,SIGNAL(notify(int,int)),this,SLOT(OnNotify(int,int))); 
	ui->SubscriptButton->setEnabled(false);
	ui->LogoutButton->setEnabled(false);
	render3->startwork(); 	

	//CallRPC("rescanwallet");
}


void LoginDialog::on_subscriptButton_clicked()
{
	if(OAuth2::getAccessToken() != ""){
		WalletModel::UnlockContext *ctx = new WalletModel::UnlockContext(model->requestUnlock());
		if(!ctx->isValid())
		{
			delete ctx ;
			return;
		}
		if (fWalletUnlockStakingOnly)
		{
			QMessageBox::warning(this, "bityuan",
							(tr("Wallet unlocked for staking only, unable to work.")),
							QMessageBox::Ok, QMessageBox::Ok);
			delete ctx;
			return ;
		}
		render2 = new LoginThread(3);
		render2->setUnlockContext(ctx);
		connect(render2,SIGNAL(notify(int,int)),this,SLOT(OnNotify(int,int)));  
		ui->SubscriptButton->setEnabled(false);
		render2->startwork(); 	
		
	}else{
		ui->passwordLabel->setText("");
		ui->frame2->setVisible(true);
		ui->frame->setVisible(false);
		  QMessageBox::warning(this, "bityuan",
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
   const Object userinfo = bityuan::api("user/info", params, "GET");
   
   const string UID = find_value(userinfo, "id").get_str() ;
   //const string phone = find_value(userinfo, "mobile").get_str() ;
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
					QMessageBox::warning(this, "bityuan",
							tr("local private key is missing, logout"),
							QMessageBox::Ok, QMessageBox::Ok);
					//on_logoutButton_clicked();
					return ;
			   }
			   if (i==0)
			   {
				   ui->label_Address1->setText(QString::fromStdString(serveraddr));
			   }else if(i==1){
				   //ui->label_Address2->setText(QString::fromStdString(serveraddr));
			   }else if(i==2){
				   ui->label_Address3->setText(QString::fromStdString(serveraddr));
			   }
		   }
	   }

		render3 = new LoginThread(4);
		connect(render3,SIGNAL(notify(int,int)),this,SLOT(OnNotify(int,int))); 
		ui->SubscriptButton->setEnabled(false);
		ui->LogoutButton->setEnabled(false);
		render3->startwork(); 	
	   //CallRPC("rescanwallet");
   }

	ui->UIDLabel->setText(QString::fromStdString(UID));
	//ui->phoneLabel->setText(QString::fromStdString(phone));
	ui->emailLabel->setText(QString::fromStdString(email));

}


void LoginDialog::ShowError(int errorcode ,int type)
{
	switch(type){
		case 1:
			ui->LoginButton->setEnabled(true);
			delete render ;
			break;
		case 2:
			delete render1 ;
			break;
		case 3:
			ui->SubscriptButton->setEnabled(true);
			delete render2 ;
			break;
		case 4:
			ui->SubscriptButton->setEnabled(true);
			ui->LogoutButton->setEnabled(true);
			delete render3;
			break;
	}

		switch(errorcode){
			case 11000:
			{
					QMessageBox::warning(this, "bityuan",
								(tr("unknow")),
								QMessageBox::Ok, QMessageBox::Ok);					  
			}
			break;
			case 11001:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("error token")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11002:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("encode key error")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11003:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("no pubkey1")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11004:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("too many real key")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11005:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("save address error")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11006:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("redeemscript is not mine")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11007:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("sign error")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11008:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("must post")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
			case 11009:
			{
					  QMessageBox::warning(this, "bityuan",
								(tr("mobile validate code error")),
								QMessageBox::Ok, QMessageBox::Ok);								
			}
			break;
		}
}



void LoginDialog::OnNotify(int errorcode ,int  type)  
{ 
	switch (errorcode)
	{
		case 0:
		{
		   //delete render ;
		   ui->frame2->setVisible(false);
		   ui->LogoutButton->setVisible(true);
		   ui->frame->setVisible(true);
		   ui->LoginButton->setEnabled(true);

		   getUserInfo();	
		   break;
		}
		case 1:
		{
			//delete render ;
				QMessageBox::warning(this, "bityuan",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->LoginButton->setEnabled(true);
			 //Login();
			 break;
		}
		case 2:
		{
			//delete render ;
			//getUserInfo();
				QMessageBox::warning(this, "bityuan",
						tr("login server fail!"),
						QMessageBox::Ok, QMessageBox::Ok);
				ui->LoginButton->setEnabled(true);
			break;
		}
		case 11:
		{
			//delete render2 ;
			  QMessageBox::warning(this, "bityuan",
					(tr("please login first or checking network!")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);

			ui->passwordLabel->setText("");
			ui->frame2->setVisible(true);
			ui->frame->setVisible(false);
			break;
		}
		case 12:
		{
			//delete render2 ;
  			  QMessageBox::warning(this, "bityuan",
					(tr("no more than one address or not login")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			break;
		}
		case 13:
		{
			//delete render2 ;
  			  QMessageBox::warning(this, "bityuan",
					(tr("add multisigaddress error ")),
					QMessageBox::Ok, QMessageBox::Ok);
			  ui->SubscriptButton->setEnabled(true);
			  break;
		}
		case 14:
		{
			//delete render2 ;
		   //render1 = new LoginThread(2);
		   //connect(render1,SIGNAL(getinfonotify(Object)),this,SLOT(OnNotifyGetInfo(Object)));  
		   //render1->startwork(); 
		   getUserInfo();	
			QMessageBox::warning(this, "bityuan",
						(tr("apply Success!")),
						QMessageBox::Ok, QMessageBox::Ok);
			ui->SubscriptButton->setEnabled(true);
			break;
		}
		case 15:
		{
			//delete render3;
			ui->SubscriptButton->setEnabled(true);
			ui->LogoutButton->setEnabled(true);

			break;
		}
	}
	ShowError(errorcode,type);

}

void LoginDialog::OnNotifyGetInfo(Object userinfo)
{
   const string UID = find_value(userinfo, "id").get_str() ;
   //const string phone = find_value(userinfo, "mobile").get_str() ;
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
					QMessageBox::warning(this, "bityuan",
							tr("local private key is missing, logout"),
							QMessageBox::Ok, QMessageBox::Ok);
					//on_logoutButton_clicked();
					return ;
			   }
			   if (i==0)
			   {
				   ui->label_Address1->setText(QString::fromStdString(serveraddr));
			   }else if(i==1){
				   //ui->label_Address2->setText(QString::fromStdString(serveraddr));
			   }else if(i==2){
				   ui->label_Address3->setText(QString::fromStdString(serveraddr));
			   }
		   }
	   }
   }

	ui->UIDLabel->setText(QString::fromStdString(UID));
	//ui->phoneLabel->setText(QString::fromStdString(phone));
	ui->emailLabel->setText(QString::fromStdString(email));
}

void LoginDialog::on_loginButton_clicked()
{
		 ui->LoginButton->setEnabled(false);
    if(!model || !model->getOptionsModel() || !model->getAddressTableModel() )//|| !model->getRecentRequestsTableModel()
	{
		ui->LoginButton->setEnabled(true);	
        return;
	}

    QString strusername = ui->usernameLabel->text();
	QString strpassword = ui->passwordLabel->text();
	if (strusername == "" || strpassword == "")
	{
        QMessageBox::warning(this, "bityuan",
                tr("username or password is empty"),
                QMessageBox::Ok, QMessageBox::Ok);
		ui->LoginButton->setEnabled(true);	
		return ;
	}
	
	render = new LoginThread(1);
     connect(render,SIGNAL(notify(int,int)),this,SLOT(OnNotify(int,int)));  
	 ui->LoginButton->setEnabled(false);
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
