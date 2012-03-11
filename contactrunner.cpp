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

#include "contactrunner.h"

#include <KDebug>
#include <KIcon>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Contact>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Types>
#include <TelepathyQt/Constants>

#include <KTp/Models/accounts-model-item.h>
#include <KTp/Models/contact-model-item.h>

ContactRunner::ContactRunner(QObject* parent, const QVariantList& args):
    Plasma::AbstractRunner(parent, args),
    m_accountsModel(0),
    m_proxyModel(0)
{
    Q_UNUSED(args);

    setObjectName("IM Contacts Runner");

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Finds all IM contacts matching :q:.")));

    Tp::registerTypes();

    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Account::FeatureCore
                                                    << Tp::Account::FeatureAvatar
                                                    << Tp::Account::FeatureCapabilities
                                                    << Tp::Account::FeatureProtocolInfo
                                                    << Tp::Account::FeatureProfile);

     Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Connection::FeatureCore
                                                    << Tp::Connection::FeatureRosterGroups
                                                    << Tp::Connection::FeatureRoster
                                                    << Tp::Connection::FeatureSelfContact);

     Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create(
                                                Tp::Features()  << Tp::Contact::FeatureAlias
                                                    << Tp::Contact::FeatureAvatarData
                                                    << Tp::Contact::FeatureSimplePresence
                                                    << Tp::Contact::FeatureCapabilities);

     Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());

    m_accountManager = Tp::AccountManager::create(accountFactory, connectionFactory, channelFactory, contactFactory);
    connect(m_accountManager->becomeReady(Tp::AccountManager::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(accountManagerReady(Tp::PendingOperation*)));
}

ContactRunner::~ContactRunner()
{
    delete m_accountsModel;
}

void ContactRunner::accountManagerReady(Tp::PendingOperation* operation)
{
    if (operation->isError()) {
        kWarning() << operation->errorMessage();
        return;
    }

    kDebug() << "Accounts manager is ready!";

    m_accountsModel = new AccountsModel(this);
    m_accountsModel->setAccountManager(m_accountManager);

    m_proxyModel = new AccountsFilterModel(this);
    m_proxyModel->setSourceModel(m_accountsModel);
    m_proxyModel->setSortMode(AccountsFilterModel::DoNotSort);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDisplayNameFilterMatchFlags(Qt::MatchContains | Qt::MatchRecursive);
    m_proxyModel->setPresenceTypeFilterFlags(AccountsFilterModel::ShowAll);
}


void ContactRunner::match(Plasma::RunnerContext& context)
{
    const QString term = context.query();
    if ((term.length() < 3) || !context.isValid()) {
        return;
    }

    if (!m_accountsModel || !m_proxyModel || !m_accountManager->isReady())
        return;

    m_proxyModel->setDisplayNameFilterString(term);

    int accountsCnt = m_proxyModel->rowCount();
    kDebug() << "Matching result in" << accountsCnt <<"accounts";
    for (int i = 0; (i < accountsCnt) && context.isValid(); i++) {

        QModelIndex accountIndex = m_proxyModel->index(i, 0);

        int contactsCount = m_proxyModel->rowCount(accountIndex);
        kDebug() << "Matching results in" << accountIndex.data(AccountsModel::DisplayNameRole).toString() << ":" << contactsCount;

        for (int j = 0; (j < contactsCount) && context.isValid(); j++) {

            QModelIndex contactIndex = m_proxyModel->index(j, 0, accountIndex);

            QString name = contactIndex.data(AccountsModel::AliasRole).toString();

            Plasma::QueryMatch match(this);

            match.setText(name.append(" (%1)").arg(accountIndex.data(AccountsModel::DisplayNameRole).toString()));
            match.setId(accountIndex.data(AccountsModel::IdRole).toString() + "," + 
                        contactIndex.data(AccountsModel::IdRole).toString());
            match.setType(Plasma::QueryMatch::ExactMatch);

            QString iconFile = contactIndex.data(AccountsModel::AvatarRole).toString();
            if (QFile::exists(iconFile)) {
                match.setIcon(QIcon(iconFile));
            } else {
                QString iconName;

                switch (contactIndex.data(AccountsModel::PresenceTypeRole).toInt()) {
                    case Tp::ConnectionPresenceTypeAvailable:
                        iconName = "im-user";
                        break;
                    case Tp::ConnectionPresenceTypeBusy:
                        iconName = "im-busy";
                        break;
                    case Tp::ConnectionPresenceTypeAway:
                    case Tp::ConnectionPresenceTypeExtendedAway:
                        iconName = "im-away";
                        break;
                    case Tp::ConnectionPresenceTypeHidden:
                    case Tp::ConnectionPresenceTypeOffline:
                        iconName = "im-offline";
                        break;
                    default:
                        iconName = "im-offline";
                }
                match.setIcon(QIcon::fromTheme("im-user"));
            }

            QString status = contactIndex.data(AccountsModel::PresenceStatusRole).toString();
            QString statusMessage = contactIndex.data(AccountsModel::PresenceMessageRole).toString();

            if (status.isEmpty() && !statusMessage.isEmpty())
                match.setSubtext(statusMessage);
            else if (!status.isEmpty() && !statusMessage.isEmpty())
                match.setSubtext(status.replace(0, 1, status.left(1).toUpper()) + " | " + statusMessage);
            else if (!status.isEmpty() && statusMessage.isEmpty())
                match.setSubtext(status.replace(0, 1, status.left(1).toUpper()));

            context.addMatch(term, match);
        }
    }
}

void ContactRunner::run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match)
{
    /* Remove the ID prefix added by Krunner */
    QString id = match.id().remove("KRunnerKTPContacts_");

    QStringList ids = id.split(",", QString::SkipEmptyParts);
    if (ids.count() != 2) {
        kWarning() << "Received invalid ID" << ids;
        return;
    }

    AccountsModelItem *accountItem = qobject_cast< AccountsModelItem* >(m_accountsModel->accountItemForId(ids.first()));
    if (!accountItem) {
        kWarning() << "Account" << ids.first() << "not found in the model!";
        return;
    }

    ContactModelItem *item = qobject_cast< ContactModelItem* >(m_accountsModel->contactItemForId(ids.first(), ids.at(1)));
    if (!item) {
        kWarning() << "Item" << match.id() << "not found in the model!";
        return;
    }

    Tp::AccountPtr account = accountItem->account();
    Tp::ContactPtr contact = item->contact();

    Tp::ChannelRequestHints hints;
    hints.setHint("org.freedesktop.Telepathy.ChannelRequest","DelegateToPreferredHandler", QVariant(true));
    account->ensureTextChat(contact, QDateTime::currentDateTime(), "org.freedesktop.Telepathy.Client.KDE.TextUi", hints);
}

#include "contactrunner.moc"
