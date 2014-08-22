#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H
#include "walletmodel.h"
#include <QDialog>
#include <QString>

namespace Ui {
    class SendCoinsDialog;
}
class WalletModel;
class SendCoinsEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE



class SendThread : public QThread  
{  
        Q_OBJECT  
signals:  
        void notify(int);  

public:
		//WalletModel *model;
private:
	int  m_type ;
	WalletModel::UnlockContext *m_ctx ;


    SendCoinsRecipient sendcoinsRecipient;
public:  
    SendThread(int type)
    {  
			m_type = type ;
    };  
	void setUnlockContext(WalletModel::UnlockContext * ctx);
	void setrecipient(SendCoinsRecipient recipient){
		sendcoinsRecipient = recipient ;
	};

	void startwork()  ;
	int SendCoins();
 

protected:
	void run() ;
};  


/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0);
    ~SendCoinsDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void pasteEntry(const SendCoinsRecipient &rv);
    bool handleURI(const QString &uri);

public slots:
    void clear();
    void reject();
    void accept();
    SendCoinsEntry *addEntry();
    void updateRemoveEnabled();
    void setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance);

private:
    Ui::SendCoinsDialog *ui;
    WalletModel *model;
    bool fNewRecipientAllowed;
	
	SendThread *render;  
	void SendCoins(QList<SendCoinsRecipient> recipients);

private slots:
    void on_sendButton_clicked();
    void removeEntry(SendCoinsEntry* entry);
    void updateDisplayUnit();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();
	void OnNotify(int type)  ;
};

#endif // SENDCOINSDIALOG_H
