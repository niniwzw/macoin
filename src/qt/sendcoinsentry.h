#ifndef SENDCOINSENTRY_H
#define SENDCOINSENTRY_H

#include <QFrame>
#include "walletmodel.h"

namespace Ui {
    class SendCoinsEntry;
}
class WalletModel;
class SendCoinsRecipient;

class GetSMSThread : public QThread  
{  
        Q_OBJECT  
signals:  
        void notify(int);  

private:
	int  m_type ;
public:  
    GetSMSThread(int type)
    {  
			m_type = type ;
    };  

	void startwork()  ;
	int GetSMSCode();

protected:
	void run() ;
};   


/** A single entry in the dialog for sending bitcoins. */
class SendCoinsEntry : public QFrame
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(QWidget *parent = 0);
    ~SendCoinsEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient &value);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void setRemoveEnabled(bool enabled);
    void clear();
	void on_checkButton_clicked();
	void OnNotify(int type) ;

signals:
    void removeEntry(SendCoinsEntry *entry);
    void payAmountChanged();

private slots:
    void on_deleteButton_clicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Ui::SendCoinsEntry *ui;
    WalletModel *model;

    SendCoinsRecipient recipient;
	GetSMSThread *render ;
    bool updateLabel(const QString &address);

	void GetSMSCode();
};

#endif // SENDCOINSENTRY_H
