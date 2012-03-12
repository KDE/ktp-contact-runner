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
#include <KFileDialog>
#include <KMimeType>

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
#include <TelepathyQt/ContactCapabilities>

#include <KTp/Models/accounts-model-item.h>
#include <KTp/Models/contact-model-item.h>
#include <KTp/presence.h>

#include <QAction>

Q_DECLARE_METATYPE(QModelIndex);

ContactRunner::ContactRunner(QObject* parent, const QVariantList& args):
    Plasma::AbstractRunner(parent, args),
    m_accountsModel(0),
    m_proxyModel(0)
{
    Q_UNUSED(args);

    setObjectName("IM Contacts Runner");

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Finds all IM contacts matching :q:.")));
    addSyntax(Plasma::RunnerSyntax("chat :q:", i18n("Finds all contacts matching :q: that are capable of text chats (default behavior)")));
    addSyntax(Plasma::RunnerSyntax("audiocall :q:", i18n("Finds all contacts matching :q: that are capable of audio call and uses audio calls as default action.")));
    addSyntax(Plasma::RunnerSyntax("videocall :q:", i18n("Finds all contacts matching :q: that are capable of video call and uses video calls as default action.")));
    addSyntax(Plasma::RunnerSyntax("sendfile :q:", i18n("Finds all contacts matching :q: that are capable of receiving files and sends file as default action.")));
    addSyntax(Plasma::RunnerSyntax("sharedesktop :q:", i18n("Finds all contacts matching :q: that are capable of sharing desktop and sets desktop sharing as default action.")));

    addAction("start-text-chat", QIcon::fromTheme("text-x-generic"), i18n("Start Chat"));
    addAction("start-audio-call", QIcon::fromTheme("voicecall"), i18n("Start Audio Call"));
    addAction("start-video-call", QIcon::fromTheme("webcamsend"), i18n("Start Video Call"));
    addAction("start-file-transfer", QIcon::fromTheme("mail-attachment"), i18n("Start Video Call"));
    addAction("start-desktop-sharing", QIcon::fromTheme("krfb"), i18n("Share My Desktop"));

    Tp::registerTypes();
    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Connection::FeatureCore
                                                    << Tp::Connection::FeatureSelfContact
                                                    << Tp::Connection::FeatureRoster);

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

QList< QAction* > ContactRunner::actionsForMatch(const Plasma::QueryMatch& match)
{
    QList< QAction* > actions;
    /* Remove the ID prefix added by Krunner */
    QString id = match.id().remove("KRunnerKTPContacts_");

    QStringList ids = id.split(",", QString::SkipEmptyParts);
    if (ids.count() != 2) {
        kWarning() << "Received invalid ID" << ids;
        return actions;
    }

    ContactModelItem *contactItem = qobject_cast< ContactModelItem* >(m_accountsModel->contactItemForId(ids.first(), ids.at(1)));
    if (!contactItem) {
        return actions;
    }

    if (contactItem->data(AccountsModel::TextChatCapabilityRole).toBool()) {
        actions.append(action("start-text-chat"));
    }

    if (contactItem->data(AccountsModel::AudioCallCapabilityRole).toBool()) {
        actions.append(action("start-audio-call"));
    }

    if (contactItem->data(AccountsModel::VideoCallCapabilityRole).toBool()) {
        actions.append(action("start-video-call"));
    }

    if (contactItem->data(AccountsModel::FileTransferCapabilityRole).toBool()) {
        actions.append(action("start-file-transfer"));
    }

    if (contactItem->data(AccountsModel::DesktopSharingCapabilityRole).toBool()) {
        actions.append(action("start-desktop-sharing"));
    }

    return actions;
}



void ContactRunner::match(Plasma::RunnerContext& context)
{
    const QString term = context.query();

    if ((term.length() < 3) || !context.isValid()) {
        return;
    }

    if (!m_accountsModel || !m_proxyModel || !m_accountManager->isReady()) {
        return;
    }

    QAction *defaultAction;
    QString contactName;
    if (term.startsWith("chat ", Qt::CaseInsensitive)) {
        defaultAction = action("start-text-chat");
        m_proxyModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByTextChatCapability);
        contactName = term.mid(5).trimmed();
    } else if (term.startsWith("audiocall ", Qt::CaseInsensitive)) {
        defaultAction = action("start-audio-call");
        m_proxyModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByAudioCallCapability);
        contactName = term.mid(10).trimmed();
    } else if (term.startsWith("videocall ", Qt::CaseInsensitive)) {
        defaultAction = action("start-video-call");
        m_proxyModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByVideoCallCapability);
        contactName = term.mid(10).trimmed();
    } else if (term.startsWith("sendfile ", Qt::CaseInsensitive)) {
        defaultAction = action("start-file-transfer");
        m_proxyModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByFileTransferCapability);
        contactName = term.mid(9).trimmed();
    } else if (term.startsWith("sharedesktop ", Qt::CaseInsensitive)) {
        defaultAction = action("start-desktop-sharing");
        m_proxyModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByDesktopSharingCapability);
        contactName = term.mid(13).trimmed();
    } else {
        defaultAction = action("start-text-chat");
        m_proxyModel->clearCapabilityFilterFlags();
        contactName = term;
    }

    m_proxyModel->setDisplayNameFilterString(contactName);

    int accountsCnt = m_proxyModel->rowCount();
    kDebug() << "Matching result in" << accountsCnt <<"accounts";
    for (int i = 0; (i < accountsCnt) && context.isValid(); i++) {

        QModelIndex accountIndex = m_proxyModel->index(i, 0);

        int contactsCount = m_proxyModel->rowCount(accountIndex);
        kDebug() << "Matching results in" << accountIndex.data(AccountsModel::DisplayNameRole).toString() << ":" << contactsCount;

        for (int j = 0; (j < contactsCount) && context.isValid(); j++) {

            Plasma::QueryMatch match(this);
            qreal relevance = 0.1;

            QModelIndex contactIndex = m_proxyModel->index(j, 0, accountIndex);

            QString name = contactIndex.data(AccountsModel::AliasRole).toString();

            match.setText(name.append(" (%1)").arg(accountIndex.data(AccountsModel::DisplayNameRole).toString()));
            match.setId(accountIndex.data(AccountsModel::IdRole).toString() + "," +
                        contactIndex.data(AccountsModel::IdRole).toString());
            match.setType(Plasma::QueryMatch::ExactMatch);

            KTp::Presence presence = contactIndex.data(AccountsModel::PresenceRole).value< KTp::Presence >();
            switch (presence.type()) {
            case Tp::ConnectionPresenceTypeAvailable:
                relevance *= 10;
                break;
            case Tp::ConnectionPresenceTypeBusy:
                relevance *= 8;
                break;
            case Tp::ConnectionPresenceTypeAway:
            case Tp::ConnectionPresenceTypeExtendedAway:
                relevance *= 6;
                break;
            case Tp::ConnectionPresenceTypeHidden:
                relevance *= 4;
                break;
            case Tp::ConnectionPresenceTypeOffline:
                relevance *= 1;
                break;
            default:
                relevance *= 1;
                break;
            }

            QString iconFile = contactIndex.data(AccountsModel::AvatarRole).toString();
            if (!iconFile.isEmpty() && QFile::exists(iconFile)) {
                match.setIcon(QIcon(iconFile));
            } else {
                match.setIcon(presence.icon());
            }

            if (!presence.statusMessage().isEmpty()) {
                match.setSubtext(presence.displayString() + " | " + presence.statusMessage());
            } else {
                match.setSubtext(presence.displayString());
            }

            match.setSelectedAction(defaultAction);
            match.setRelevance(relevance);

            context.addMatch(term, match);
        }
    }
}

void ContactRunner::run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match)
{
    Q_UNUSED(context)

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

    if (match.selectedAction() == action("start-text-chat")) {

        account->ensureTextChat(contact,
                                QDateTime::currentDateTime(),
                                "org.freedesktop.Telepathy.Client.KTp.TextUi",
                                hints);

    } else if (match.selectedAction() == action("start-audio-call")) {

        account->ensureStreamedMediaAudioCall(contact,
                                              QDateTime::currentDateTime(),
                                              "org.freedesktop.Telepathy.Client.KTp.CallUi");

    } else if (match.selectedAction() == action("start-video-call")) {

        account->ensureStreamedMediaVideoCall(contact,
                                              true,
                                              QDateTime::currentDateTime(),
                                              "org.freedesktop.Telepathy.Client.KTp.CallUi");

    } else if (match.selectedAction() == action("start-file-transfer")) {

        QStringList filenames = KFileDialog::getOpenFileNames(
            KUrl("kfiledialog:///FileTransferLastDirectory"),
            QString(),
            0,
            i18n("Choose files to send to %1", contact->alias()));

        if (filenames.isEmpty()) { // User hit cancel button
            return;
        }

        foreach (const QString &filename, filenames) {
            Tp::FileTransferChannelCreationProperties properties(
                filename, KMimeType::findByFileContent(filename)->name());

            account->createFileTransfer(contact,
                                        properties,
                                        QDateTime::currentDateTime(),
                                        "org.freedesktop.Telepathy.Client.KTp.FileTransfer");
        }

    } else if (match.selectedAction() == action("start-desktop-sharing")) {

        account->createStreamTube(contact, 
                                  QLatin1String("rfb"),
                                  QDateTime::currentDateTime(),
                                  "org.freedesktop.Telepathy.Client.krfb_rfb_handler");

    }
}

#include "contactrunner.moc"
