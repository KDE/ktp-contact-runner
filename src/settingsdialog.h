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


#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtGui/QWidget>
#include <KConfigGroup>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingOperation>

#include <KTp/Models/accounts-model.h>

#include <KCModule>

class QTreeView;
class QSortFilterProxyModel;
class QAbstractItemModel;
class KPushButton;

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog: public KCModule
{
    Q_OBJECT
  public:
    explicit SettingsDialog(QWidget *parent = 0, const QVariantList& args = QVariantList());
    virtual ~SettingsDialog();

  public Q_SLOTS:
    void load();
    void save();
    void defaults();

  private Q_SLOTS:
    void moveTop();
    void moveUp();
    void moveDown();
    void moveBottom();

    void selectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void accountManagerReady(Tp::PendingOperation *);

  private:
    QTreeView *m_treeView;
    QSortFilterProxyModel *m_proxy;
    QAbstractItemModel *m_model;

    AccountsModel *m_accountsModel;
    Tp::AccountManagerPtr m_accountManager;

    KPushButton *m_topButton;
    KPushButton *m_upButton;
    KPushButton *m_downButton;
    KPushButton *m_bottomButton;

    void debug();
};

#endif // SETTINGSDIALOG_H
