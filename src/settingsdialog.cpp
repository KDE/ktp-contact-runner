/*
    Copyright (C) 2012  Dan Vratil <dan@progdan.cz>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "settingsdialog.h"

#include <QtGui/QGridLayout>
#include <KPushButton>
#include <QtGui/QTreeView>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingReady>

#include <KDebug>
#include <KComponentData>
#include <Plasma/AbstractRunner>
#include <KPushButton>
#include <KLocalizedString>

K_EXPORT_RUNNER_CONFIG(KtpRunnerSettings, SettingsDialog)

static const int SortRole = AccountsModel::CustomRole;

SettingsDialog::SettingsDialog(QWidget *parent, const QVariantList& args):
    KCModule(ConfigFactory::componentData(), parent, args),
    m_accountsModel(0)
{
    QGridLayout *layout = new QGridLayout(parent);
    setLayout(layout);

    QLabel *label = new QLabel(i18n("Order accounts by your preference:"), this);
    layout->addWidget(label, 0, 0, 1, 2);

    m_treeView = new QTreeView(this);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->header()->hide();
    m_treeView->setDragDropMode(QAbstractItemView::InternalMove);
    m_treeView->setDragEnabled(true);
    m_treeView->viewport()->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    layout->addWidget(m_treeView, 1, 0, 4, 1);

    m_topButton = new KPushButton(KIcon("arrow-up-double"), QString(), this);
    m_topButton->setDisabled(true);
    layout->addWidget(m_topButton, 1, 1);
    connect(m_topButton, SIGNAL(clicked(bool)), this, SLOT(moveTop()));

    m_upButton = new KPushButton(KIcon("arrow-up"), QString());
    m_upButton->setDisabled(true);
    layout->addWidget(m_upButton, 2, 1);
    connect(m_upButton, SIGNAL(clicked(bool)), this, SLOT(moveUp()));

    m_downButton = new KPushButton(KIcon("arrow-down"), QString());
    m_downButton->setDisabled(true);
    layout->addWidget(m_downButton, 3, 1);
    connect(m_downButton, SIGNAL(clicked(bool)), this, SLOT(moveDown()));

    m_bottomButton = new KPushButton(KIcon("arrow-down-double"), QString());
    m_bottomButton->setDisabled(true);
    layout->addWidget(m_bottomButton, 4, 1);
    connect(m_bottomButton, SIGNAL(clicked(bool)), this, SLOT(moveBottom()));


    Tp::registerTypes();
    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Connection::FeatureCore
                                                    << Tp::Connection::FeatureSelfContact);


    m_accountManager = Tp::AccountManager::create(accountFactory, connectionFactory);
    connect(m_accountManager->becomeReady(Tp::AccountManager::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation *)),
            this, SLOT(accountManagerReady(Tp::PendingOperation *)));
}

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::accountManagerReady(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        kWarning() << operation->errorMessage();
        return;
    }

    kDebug() << "Accounts manager is ready!";

    m_accountsModel = new AccountsModel(this);
    m_accountsModel->setAccountManager(m_accountManager);

    load();

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_accountsModel);
    m_proxy->setSortRole(SortRole);

    m_treeView->setModel(m_proxy);
    connect(m_treeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionChanged(QModelIndex,QModelIndex)));
}

void SettingsDialog::selectionChanged(const QModelIndex &current , const QModelIndex &previous)
{
    if (!current.isValid()) {
        return;
    }

    m_topButton->setEnabled(current.row() != 0);
    m_upButton->setEnabled(current.row() != 0);

    m_downButton->setEnabled(current.row() < current.model()->rowCount(current.parent()) - 1);
    m_bottomButton->setEnabled(current.row() < current.model()->rowCount(current.parent()) - 1);

    Q_UNUSED(previous);
}

void SettingsDialog::moveTop()
{
    QItemSelectionModel *selection = m_treeView->selectionModel();

    QModelIndex index = selection->currentIndex();
    int r = index.row();

    for (int i = 0; i < r; i++) {
        m_proxy->setData(m_proxy->index(i, 0), i + 1, SortRole);
    }

    m_proxy->setData(index, 0, SortRole);

    m_proxy->sort(0, Qt::AscendingOrder);
}

void SettingsDialog::moveUp()
{
    QItemSelectionModel *selection = m_treeView->selectionModel();

    QModelIndex index = selection->currentIndex();
    QModelIndex prev = index.sibling(index.row() - 1, 0);
    m_proxy->setData(index, m_proxy->data(index, SortRole).toInt() - 1, SortRole);
    m_proxy->setData(prev, m_proxy->data(prev, SortRole).toInt() + 1, SortRole);

    m_proxy->sort(0, Qt::AscendingOrder);

    debug();
}

void SettingsDialog::moveDown()
{
    QItemSelectionModel *selection = m_treeView->selectionModel();

    QModelIndex index = selection->currentIndex();
    QModelIndex prev = index.sibling(index.row() + 1, 0);
    m_proxy->setData(index, m_proxy->data(index, SortRole).toInt() + 1, SortRole);
    m_proxy->setData(prev, m_proxy->data(prev, SortRole).toInt() - 1, SortRole);

    m_proxy->sort(0, Qt::AscendingOrder);

    debug();
}

void SettingsDialog::moveBottom()
{
    QItemSelectionModel *selection = m_treeView->selectionModel();

    QModelIndex index = selection->currentIndex();
    int r = index.row();

    for (int i = r + 1; i < m_proxy->rowCount(); i++) {
        m_proxy->setData(m_proxy->index(i, 0), i - 1, SortRole);
    }

    m_proxy->setData(index, m_proxy->rowCount() - 1, SortRole);

    m_proxy->sort(0, Qt::AscendingOrder);

    debug();
}

void SettingsDialog::load()
{
    if (!m_accountsModel)
        return;

    KCModule::load();

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig("krunnerrc");
    KConfigGroup grp = cfg->group("Runners");
    grp = KConfigGroup(&grp, "KTp Contact Runner");

    for (int i = 0; i < m_accountsModel->rowCount(); i++) {
        m_accountsModel->setData(m_accountsModel->index(i, 0), i, SortRole);
    }
    //QStringList s = grp.readEntry("preferences", QStringList());

    debug();
}

void SettingsDialog::save()
{
    KCModule::save();
}

void SettingsDialog::defaults()
{
    KCModule::defaults();
}

void SettingsDialog::debug()
{
    for (int i = 0; i < m_accountsModel->rowCount(); i++) {
        qDebug() << m_accountsModel->data(m_accountsModel->index(i, 0), Qt::DisplayRole) << " - " << m_accountsModel->data(m_accountsModel->index(i, 0), SortRole);
    }
}
