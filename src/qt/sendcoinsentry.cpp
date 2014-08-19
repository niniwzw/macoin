#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"
#include "guiutil.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"

#include <QApplication>
#include <QClipboard>


#include "base58.h"
#include "bitcoinrpc.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace json_spirit;


 

void GetSMSThread::startwork()  
{  
	start(); //HighestPriority 
}  


int GetSMSThread::GetSMSCode()
{
		try{
			const Object reto = Macoin::sendRandCode();
			
			Value errorObj = find_value(reto,  "error");
			if (errorObj.type() == null_type)
			{
				return 1;
			}
			const string strerror = errorObj.get_str(); 
			
		}catch(QString exception){
			return 2 ;
		}
		return 0;
}

void GetSMSThread::run()  
{ 
	int ret =-1;
	switch(m_type){
		case 4:
		{
			ret = GetSMSCode();
			emit notify(ret);  
			break;
		}
	}
}  



SendCoinsEntry::SendCoinsEntry(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SendCoinsEntry),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    ui->payToLayout->setSpacing(4);
#endif
#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
    ui->payTo->setPlaceholderText(tr("Enter a Macoin address (e.g. 1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L)"));
#endif
    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(ui->payTo);

    GUIUtil::setupAddressWidget(ui->payTo, this);
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
    if(!model)
        return;
    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
        ui->addAsLabel->setText(associatedLabel);
}

void SendCoinsEntry::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    connect(ui->payAmount, SIGNAL(textChanged()), this, SIGNAL(payAmountChanged()));

    clear();
}

void SendCoinsEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void SendCoinsEntry::clear()
{
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->payTo->setFocus();
    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void SendCoinsEntry::on_deleteButton_clicked()
{
    emit removeEntry(this);
}

bool SendCoinsEntry::validate()
{
    // Check input validity
    bool retval = true;

    if(!ui->payAmount->validate())
    {
        retval = false;
    }
    else
    {
        if(ui->payAmount->value() <= 0)
        {
            // Cannot send 0 coins or less
            ui->payAmount->setValid(false);
            retval = false;
        }
    }

    if(!ui->payTo->hasAcceptableInput() ||
       (model && !model->validateAddress(ui->payTo->text())))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    SendCoinsRecipient rv;

    rv.address = ui->payTo->text();
    rv.label = ui->addAsLabel->text();
    rv.amount = ui->payAmount->value();
    //rv.message = ui->messageTextLabel->text();
    rv.smsverifycode = ui->checkSMS->text();
	rv.stramount = ui->payAmount->text();
    return rv;
}

void SendCoinsEntry::on_checkButton_clicked()
{
	if(OAuth2::getAccessToken() != ""){

		 render = new GetSMSThread(4);
		 connect(render,SIGNAL(notify(int)),this,SLOT(OnNotify(int)));  
		 ui->checkButton->setEnabled(false);
		 render->startwork();    

	}else{
		  QMessageBox::warning(this, "macoin",
                (tr("please login first!")),
                QMessageBox::Ok, QMessageBox::Ok);
	}
}


void SendCoinsEntry::OnNotify(int type)  
{  
	switch (type)
	{
		case 0:
		{
			  QMessageBox::warning(this, "macoin",
					QString::fromStdString("sms send fail"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
				break;
		case 1:
		{
				QMessageBox::warning(this, "macoin",
					tr("sms sending complete!"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
			 break;
		case 2:
		{
			QMessageBox::warning(this, "macoin",
                "sms send exception",
                QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
				break;
	}
}


QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    QWidget::setTabOrder(ui->deleteButton, ui->addAsLabel);
    return ui->payAmount->setupTabChain(ui->addAsLabel);
}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    ui->payTo->setText(value.address);
    ui->addAsLabel->setText(value.label);
    ui->payAmount->setValue(value.amount);
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}

void SendCoinsEntry::GetSMSCode()
{
		try{
			const Object reto = Macoin::sendRandCode();
			
			Value errorObj = find_value(reto,  "error");
			if (errorObj.type() == null_type)
			{
				QMessageBox::warning(this, "macoin",
					tr("sms sending complete!"),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
				return ;
			}
			const string strerror = errorObj.get_str(); 
			  QMessageBox::warning(this, "macoin",
					QString::fromStdString(strerror),
					QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
			
		}catch(QString exception){
			QMessageBox::warning(this, "macoin",
                exception,
                QMessageBox::Ok, QMessageBox::Ok);
				ui->checkButton->setEnabled(true);
		}
		return ;
}
