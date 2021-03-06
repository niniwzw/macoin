#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "init.h"

#include "addresstablemodel.h"
#include "addressbookpage.h"

#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"

#include "coincontrol.h"
#include "coincontroldialog.h"

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>




#include "bitcoinrpc.h"

#include "base58.h"

#include <boost/algorithm/string.hpp>



using namespace std;
using namespace json_spirit;

extern bool fWalletUnlockStakingOnly;

Value CallRPC1(string args)
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



void SendThread::startwork()  
{  
	start(); //HighestPriority 
}  

void SendThread::setUnlockContext(WalletModel::UnlockContext * ctx)
{
	m_ctx = ctx ;
}


int SendThread::SendCoins()
{
		try{
				const Object transactionObj  = bityuan::createrawtransaction(sendcoinsRecipient.address.toStdString(), sendcoinsRecipient.stramount.toStdString(), sendcoinsRecipient.smsverifycode.toStdString());
				delete m_ctx;
				m_ctx = NULL ;
				Value retvalue = find_value(transactionObj , "nologin");
				if (retvalue.type() == str_type)
				{
					return 8;
				}
					
				Value errorvalue = find_value(transactionObj , "error");
				if (errorvalue.type() != null_type)
				{
					Value codevalue = find_value(transactionObj , "code");
					if (codevalue.type() == int_type){
						return codevalue.get_int();
					}
					return 1;
				}
				
				Value rawValue = find_value(transactionObj , "hex");
				if (rawValue.type() == null_type)
				{
					return 2;
				}
				string raw = rawValue.get_str();
				Value completeValue = find_value(transactionObj , "complete");
				if (completeValue.type() !=  bool_type)
				{
					return 4;
				}
				bool complete = completeValue.get_bool();
				if (complete == true) {

					if (rawValue.type() == null_type)
					{
						return 5;
					}
					string hex = rawValue.get_str();
					Value callrpc = CallRPC1(string("sendrawtransaction ") + hex);
					return 0 ;
				}else{
					return 6  ;
				}
			}catch(...){
				if(m_ctx != NULL){
					delete m_ctx;
					m_ctx = NULL;
				}
				return 7 ;
			}
}

void SendThread::run()  
{ 
	int ret =-1;
	switch(m_type){
		case 5:
		{
			ret = SendCoins();
			emit notify(ret);  
			break;
		}
	}
}  




SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    //ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->lineEditCoinControlChange->setPlaceholderText(tr("Enter a bityuan address (e.g. 1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L)"));
#endif

    addEntry();

    //connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    ui->lineEditCoinControlChange->setFont(GUIUtil::bitcoinAddressFont());
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    fNewRecipientAllowed = true;
}

void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setModel(model);
        }
    }
    if(model && model->getOptionsModel())
    {
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::SendCoins(QList<SendCoinsRecipient> recipients)
{
		SendCoinsRecipient sendcoinsRecipient = (SendCoinsRecipient)recipients.takeAt(0);
		try{
				const Object transactionObj  = bityuan::createrawtransaction(sendcoinsRecipient.address.toStdString(), sendcoinsRecipient.stramount.toStdString(), sendcoinsRecipient.smsverifycode.toStdString());
				Value retvalue = find_value(transactionObj , "nologin");
				if (retvalue.type() == str_type)
				{
					model->showLoginView();
					QMessageBox::warning(this, "bityuan",
							(tr("please login first!")),
							QMessageBox::Ok, QMessageBox::Ok);
					return ;
				}
										
				Value errorobj = find_value(transactionObj , "error");
				if (errorobj.type() != null_type)
				{
 				  ui->sendButton->setEnabled(true);
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString(errorobj.get_str()),
							QMessageBox::Ok, QMessageBox::Ok);

					return;
				}
				
				Value rawValue = find_value(transactionObj , "hex");
				if (rawValue.type() == null_type)
				{
 				  ui->sendButton->setEnabled(true);
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("hex"),
							QMessageBox::Ok, QMessageBox::Ok);
					return ;
				}
				//string raw = rawValue.get_str();
				/*
				Value rpcobj = CallRPC1(string("signrawtransaction ") + raw);
				if (rpcobj.type() != obj_type)
				{
 				  ui->sendButton->setEnabled(true);
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("signrawtransaction"),
							QMessageBox::Ok, QMessageBox::Ok);
					return ;
				}

				const Object resultobj = rpcobj.get_obj();
				*/

				Value completeValue = find_value(transactionObj , "complete");
				if (completeValue.type() !=  bool_type)
				{
 				  ui->sendButton->setEnabled(true);
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("complete"),
							QMessageBox::Ok, QMessageBox::Ok);
					return;
				}
				bool complete = completeValue.get_bool();
				  
				//Value hexValue = find_value(resultobj , "hex");
				if (complete == true) {

					if (rawValue.type() == null_type)
					{
	 				  ui->sendButton->setEnabled(true);
					  QMessageBox::warning(this, "bityuan",
								QString::fromStdString("hex1"),
								QMessageBox::Ok, QMessageBox::Ok);
						return ;
					}
					string hex = rawValue.get_str();
					Value callrpc = CallRPC1(string("sendrawtransaction ") + hex);
				}else{
	 				  ui->sendButton->setEnabled(true);
					  QMessageBox::warning(this, "bityuan",
								tr("sending fail ") + QString::fromStdString(rawValue.get_str()),
								QMessageBox::Ok, QMessageBox::Ok);
				}

			}catch(...){
 				  ui->sendButton->setEnabled(true);
				QMessageBox::warning(this, "bityuan",
					(tr("some exception")),
					QMessageBox::Ok, QMessageBox::Ok);

			}
}


void SendCoinsDialog::ShowError(int type)
{

		switch(type){
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


void SendCoinsDialog::OnNotify(int type)  
{  
	ui->sendButton->setEnabled(true);
	delete render ;
	switch(type){
		case 0:
		{
 				  
		}
			break;
		case 1:
		{
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("createrawtransaction find error is fail"),
							QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 2:
		{
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("createrawtransaction find hex is fail"),
							QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 3:
		{
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("signrawtransaction is fail"),
							QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 4:
		{
				  QMessageBox::warning(this, "bityuan",
							QString::fromStdString("signrawtransaction find complete is fail"),
							QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 5:
		{
					  QMessageBox::warning(this, "bityuan",
								QString::fromStdString("signrawtransaction find hex is fail"),
								QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 6:
		{
					  QMessageBox::warning(this, "bityuan",
								tr("sending fail "),
								QMessageBox::Ok, QMessageBox::Ok);

		}
			break;
		case 7:
		{

				QMessageBox::warning(this, "bityuan",
					(tr("some exception")),
					QMessageBox::Ok, QMessageBox::Ok);
		}
			break;
		case 8:
		{

				model->showLoginView();
				QMessageBox::warning(this, "bityuan",
						(tr("please login first!")),
						QMessageBox::Ok, QMessageBox::Ok);
		}
			break;

	}
	ShowError(type);
}



void SendCoinsDialog::on_sendButton_clicked()
{
    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    // Format confirmation message
    QStringList formatted;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, rcp.amount), Qt::escape(rcp.label), rcp.address));
    }

    fNewRecipientAllowed = false;

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
                          tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    WalletModel::UnlockContext *ctx= new WalletModel::UnlockContext(model->requestUnlock());
    if(!ctx->isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
		delete ctx ;
        return;
    }

    if (fWalletUnlockStakingOnly)
    {
		QMessageBox::warning(this, "bityuan",
						(tr("Wallet unlocked for staking only, unable to create transaction.")),
						QMessageBox::Ok, QMessageBox::Ok);  
		fNewRecipientAllowed = true;
		delete ctx ;
        return ;
    }
	///////////////////////////////////////////////////////////////////////////

	render = new SendThread(5);
	render->setUnlockContext(ctx);
	connect(render,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
	ui->sendButton->setEnabled(false);
	SendCoinsRecipient sendcoinsRecipient = (SendCoinsRecipient)recipients.takeAt(0);
	render->setrecipient(sendcoinsRecipient);
	render->startwork();    
		

	return ;
	
	/////////////////////////////////////////////////////////////////////////////////

    WalletModel::SendCoinsReturn sendstatus;

    if (!model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures()){
        sendstatus = model->sendCoins(recipients);
		}
    else{
        sendstatus = model->sendCoins(recipients, CoinControlDialog::coinControl);

		}

    switch(sendstatus.status)
    {
    case WalletModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::InvalidAmount:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount exceeds your balance."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The total exceeds your balance when the %1 transaction fee is included.").
            arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::DuplicateAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCreationFailed:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Error: Transaction creation failed."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCommitFailed:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::Aborted: // User aborted, nothing to do
        break;
    case WalletModel::OK:
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
        break;
    }
    fNewRecipientAllowed = true;
}

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        delete ui->entries->takeAt(0)->widget();
    }
    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendCoinsDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setRemoveEnabled(enabled);
        }
    }
    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    delete entry;
    updateRemoveEnabled();
}

QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    //QWidget::setTabOrder(prev, ui->addButton);
    //QWidget::setTabOrder(ui->addButton, ui->sendButton);
	QWidget::setTabOrder(prev, ui->sendButton);
    return ui->sendButton;
}

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
}

bool SendCoinsDialog::handleURI(const QString &uri)
{
    SendCoinsRecipient rv;
    // URI has to be valid
    if (GUIUtil::parseBitcoinURI(uri, &rv))
    {
        CBitcoinAddress address(rv.address.toStdString());
        if (!address.IsValid())
            return false;
        pasteEntry(rv);
        return true;
    }

    return false;
}

void SendCoinsDialog::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(stake);
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    if(!model || !model->getOptionsModel())
        return;

    int unit = model->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
}

void SendCoinsDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}

// Coin Control: copy label "Quantity" to clipboard
void SendCoinsDialog::coinControlClipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsDialog::coinControlClipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsDialog::coinControlClipboardFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsDialog::coinControlClipboardBytes()
{
    QApplication::clipboard()->setText(ui->labelCoinControlBytes->text());
}

// Coin Control: copy label "Priority" to clipboard
void SendCoinsDialog::coinControlClipboardPriority()
{
    QApplication::clipboard()->setText(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Low output" to clipboard
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    QApplication::clipboard()->setText(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsDialog::coinControlClipboardChange()
{
    QApplication::clipboard()->setText(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    if (model)
    {
        if (state == Qt::Checked)
            CoinControlDialog::coinControl->destChange = CBitcoinAddress(ui->lineEditCoinControlChange->text().toStdString()).Get();
        else
            CoinControlDialog::coinControl->destChange = CNoDestination();
    }

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
    ui->labelCoinControlChangeLabel->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString & text)
{
    if (model)
    {
        CoinControlDialog::coinControl->destChange = CBitcoinAddress(text.toStdString()).Get();

        // label for the change address
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");
        if (text.isEmpty())
            ui->labelCoinControlChangeLabel->setText("");
        else if (!CBitcoinAddress(text.toStdString()).IsValid())
        {
            ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
            ui->labelCoinControlChangeLabel->setText(tr("WARNING: Invalid bityuan address"));
        }
        else
        {
            QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
            if (!associatedLabel.isEmpty())
                ui->labelCoinControlChangeLabel->setText(associatedLabel);
            else
            {
                CPubKey pubkey;
                CKeyID keyid;
                CBitcoinAddress(text.toStdString()).GetKeyID(keyid);   
                if (model->getPubKey(keyid, pubkey))
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));
                else
                {
                    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
                    ui->labelCoinControlChangeLabel->setText(tr("WARNING: unknown change address"));
                }
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}