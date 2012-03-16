/*
    Copyright (C) 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KTPCONTACTRUNNER_H
#define KTPCONTACTRUNNER_H

#include <Plasma/AbstractRunner>
#include <KIcon>

#include <KTp/Models/accounts-model.h>
#include <KTp/Models/accounts-filter-model.h>
#include <KTp/Models/groups-model.h>
#include <KTp/Models/flat-model-proxy.h>
#include <TelepathyQt/AccountManager>

class QAction;

namespace Tp {
    class PendingOperation;
}

class ContactRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

  public:
    ContactRunner( QObject *parent, const QVariantList& args );
    ~ContactRunner();

    void match(Plasma::RunnerContext &context);
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

  protected:
    QList< QAction* > actionsForMatch(const Plasma::QueryMatch &match);

  private Q_SLOTS:
    void accountManagerReady(Tp::PendingOperation *operation);

  private:
    AccountsModel *m_accountsModel;
    FlatModelProxy *m_flatModelProxy;
    AccountsFilterModel *m_proxyModel;
    Tp::AccountManagerPtr m_accountManager;
};

K_EXPORT_PLASMA_RUNNER(KRunnerKTPContacts, ContactRunner)

#endif /* KTPCONTACTRUNNER_H */
