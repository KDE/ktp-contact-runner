/*
    KTp Contact Runner
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

#include "contactrunner.h"

#include <KDebug>
#include <KFileDialog>
#include <KMimeType>
#include <KToolInvocation>
#include <KStandardDirs>
#include <KLocale>

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

#include <KTp/presence.h>
#include <KTp/global-presence.h>
#include <KTp/actions.h>
#include <KTp/contact-factory.h>
#include <KTp/contact.h>

struct MatchInfo {
    Tp::AccountPtr account;
    Tp::ContactPtr contact;
    KTp::Presence presence;
};

Q_DECLARE_METATYPE(QModelIndex);
Q_DECLARE_METATYPE(MatchInfo);

ContactRunner::ContactRunner(QObject *parent, const QVariantList &args):
    Plasma::AbstractRunner(parent, args),
    m_globalPresence(new KTp::GlobalPresence(this))
{
    Q_UNUSED(args);

    setObjectName(QLatin1String("IM Contacts Runner"));

    m_loggerDisabled = KStandardDirs::findExe(QLatin1String("ktp-log-viewer")).isEmpty();

    addSyntax(Plasma::RunnerSyntax(QLatin1String(":q:"), i18n("Finds all IM contacts matching :q:.")));
    addSyntax(Plasma::RunnerSyntax(QLatin1String("chat :q:"), i18n("Finds all contacts matching :q: that are capable of text chats (default behavior)")));
    addSyntax(Plasma::RunnerSyntax(QLatin1String("audiocall :q:"), i18n("Finds all contacts matching :q: that are capable of audio call and uses audio calls as default action.")));
    addSyntax(Plasma::RunnerSyntax(QLatin1String("videocall :q:"), i18n("Finds all contacts matching :q: that are capable of video call and uses video calls as default action.")));
    addSyntax(Plasma::RunnerSyntax(QLatin1String("sendfile :q:"), i18n("Finds all contacts matching :q: that are capable of receiving files and sends file as default action.")));
    addSyntax(Plasma::RunnerSyntax(QLatin1String("sharedesktop :q:"), i18n("Finds all contacts matching :q: that are capable of sharing desktop and sets desktop sharing as default action.")));

    if (!m_loggerDisabled) {
        addSyntax(Plasma::RunnerSyntax(QLatin1String("log :q:"), i18n("Open the log viewer for :q:")));
    }

    QString imKeyword = i18nc("A keyword to change IM status", "im") + QLatin1String(" :q:");
    QString statusKeyword = i18nc("A keyword to change IM status", "status") + QLatin1String(" :q:");
    Plasma::RunnerSyntax presenceSyntax(imKeyword, i18n("Change IM status"));
    presenceSyntax.addExampleQuery(statusKeyword);
    presenceSyntax.setSearchTermDescription(i18nc("Search term description", "status"));
    addSyntax(presenceSyntax);

    Plasma::RunnerSyntax presenceMsgSyntax(imKeyword + ' ' + i18nc("Description of a search term, please keep the brackets", "<status message>"),
                                           i18n("Change IM status and set status message."));
    presenceMsgSyntax.addExampleQuery(statusKeyword + ' ' + i18nc("Description of a search term, please keep the brackets", "<status message>"));
    presenceMsgSyntax.setSearchTermDescription(i18nc("Search term description", "status"));
    addSyntax(presenceMsgSyntax);

    addSyntax(Plasma::RunnerSyntax(i18nc("A command to connect all IM accounts", "connect"),
                                   i18n("Connect all IM accounts")));
    addSyntax(Plasma::RunnerSyntax(i18nc("A command to disconnect all IM accounts", "disconnect"),
                                   i18n("Disconnect all IM accounts")));;

    addAction(QLatin1String("start-text-chat"), QIcon::fromTheme(QLatin1String("text-x-generic")), i18n("Start Chat"));
    addAction(QLatin1String("start-audio-call"), QIcon::fromTheme(QLatin1String("audio-headset")), i18n("Start Audio Call"));
    addAction(QLatin1String("start-video-call"), QIcon::fromTheme(QLatin1String("camera-web")), i18n("Start Video Call"));
    addAction(QLatin1String("start-file-transfer"), QIcon::fromTheme(QLatin1String("mail-attachment")), i18n("Send file(s)"));
    addAction(QLatin1String("start-desktop-sharing"), QIcon::fromTheme(QLatin1String("krfb")), i18n("Share My Desktop"));

    if (!m_loggerDisabled) {
        addAction(QLatin1String("show-log-viewer"), QIcon::fromTheme(QLatin1String("view-pim-journal")), i18n("Open the log viewer"));
    }

    Tp::registerTypes();
    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                                                QDBusConnection::sessionBus(),
                                                Tp::Features() << Tp::Connection::FeatureCore
                                                    << Tp::Connection::FeatureSelfContact
                                                    << Tp::Connection::FeatureRoster);

    Tp::ContactFactoryPtr contactFactory = KTp::ContactFactory::create(
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

}

void ContactRunner::accountManagerReady(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        kWarning() << operation->errorMessage();
        return;
    }

    kDebug() << "Accounts manager is ready!";

    m_globalPresence->setAccountManager(m_accountManager);
}

QList< QAction* > ContactRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    QList< QAction* > actions;

    MatchInfo data = match.data().value< MatchInfo >();
    if (!data.contact) {
        return actions;
    }

    if (hasCapability(data.contact, ContactRunner::TextChatCapability)) {
        actions.append(action(QLatin1String("start-text-chat")));

        if (!m_loggerDisabled) {
            actions.append(action(QLatin1String("show-log-viewer")));
        }
    }

    if (hasCapability(data.contact, ContactRunner::AudioCallCapability)) {
        actions.append(action(QLatin1String("start-audio-call")));
    }

    if (hasCapability(data.contact, ContactRunner::VideoCallCapability)) {
        actions.append(action(QLatin1String("start-video-call")));
    }

    if (hasCapability(data.contact, ContactRunner::FileTransferCapability)) {
        actions.append(action(QLatin1String("start-file-transfer")));
    }

    if (hasCapability(data.contact, ContactRunner::DesktopSharingCapability)) {
        actions.append(action(QLatin1String("start-desktop-sharing")));
    }

    return actions;
}



void ContactRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();

    if ((term.length() < 3) || !context.isValid()) {
        return;
    }

    if (!m_accountManager->isReady()) {
        return;
    }

    /* First check for "im" or "status" keywords */
    matchPresence(context);

    /* But always try to match contacts too (in case somebody had a contact
     * names "im..." */
    matchContacts(context);
}


void ContactRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    MatchInfo data = match.data().value< MatchInfo >();
    if (data.presence.isValid()) {
        data.presence.setStatus(data.presence.type(), data.presence.status(), data.presence.statusMessage());
        m_globalPresence->setPresence(data.presence);

        return;
    }

    if (!data.account || !data.contact) {
        kWarning() << "Running invalid contact info";
        return;
    }

    /* Open chat/call/whatever with contact */
    Tp::AccountPtr account = data.account;
    Tp::ContactPtr contact = data.contact;

    if (match.selectedAction() == action(QLatin1String("start-text-chat"))) {
        KTp::Actions::startChat(account, contact);
    } else if (match.selectedAction() == action(QLatin1String("start-audio-call"))) {
        KTp::Actions::startAudioCall(account, contact);
    } else if (match.selectedAction() == action("start-video-call")) {
        KTp::Actions::startAudioVideoCall(account, contact);
    } else if (match.selectedAction() == action("start-file-transfer")) {

      QStringList filenames = KFileDialog::getOpenFileNames(
                                    KUrl("kfiledialog:///FileTransferLastDirectory"),
                                    QString(),
                                    0,
                                    i18n("Choose files to send to %1", contact->alias()));

        if (filenames.isEmpty()) { // User hit cancel button
            return;
        }

        Q_FOREACH (const QString &filename, filenames) {
            KTp::Actions::startFileTransfer(account, contact, filename);
        }

    } else if (match.selectedAction() == action("start-desktop-sharing")) {
        KTp::Actions::startDesktopSharing(account, contact);
    } else if (match.selectedAction() == action(QLatin1String("show-log-viewer"))) {
        KTp::Actions::openLogViewer(account, contact);
    }
}

bool ContactRunner::hasCapability(const Tp::ContactPtr &contact, Capability capability) const
{
    if (capability == AllCapabilitites) {
        return true;
    }

    if ((capability == TextChatCapability) && contact->capabilities().textChats()) {
        return true;
    }

    const KTp::ContactPtr ktpContact = KTp::ContactPtr::dynamicCast(contact);

    if ((capability == AudioCallCapability) && ktpContact->audioCallCapability()) {
        return true;
    }

    if ((capability == VideoCallCapability) && ktpContact->videoCallCapability()) {
        return true;
    }

    if ((capability == FileTransferCapability) && ktpContact->fileTransferCapability()) {
        return true;
    }

    if ((capability == DesktopSharingCapability) &&
        contact->capabilities().streamTubes(QLatin1String("org.freedesktop.Telepathy.Client.krfb_rfb_handler"))) {
        return true;
    }

    return false;
}

void ContactRunner::matchContacts(Plasma::RunnerContext &context)
{
    QString term = context.query();

    QAction *defaultAction;
    QString contactQuery;
    Capability capability;
    if (term.startsWith(QLatin1String("chat "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("start-text-chat"));
        capability = TextChatCapability;
        contactQuery = term.mid(QString("chat").length()).trimmed();
    } else if (term.startsWith(QLatin1String("audiocall "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("start-audio-call"));
        capability = AudioCallCapability;
        contactQuery = term.mid(QString("audiocall").length()).trimmed();
    } else if (term.startsWith(QLatin1String("videocall "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("start-video-call"));
        capability = VideoCallCapability;
        contactQuery = term.mid(QString("videocall").length()).trimmed();
    } else if (term.startsWith(QLatin1String("sendfile "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("start-file-transfer"));
        capability = FileTransferCapability;
        contactQuery = term.mid(QString("sendfile").length()).trimmed();
    } else if (term.startsWith(QLatin1String("sharedesktop "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("start-desktop-sharing"));
        capability = DesktopSharingCapability;
        contactQuery = term.mid(QString("sharedesktop").length()).trimmed();
    } else if (term.startsWith(QLatin1String("log "), Qt::CaseInsensitive)) {
        defaultAction = action(QLatin1String("show-log-viewer"));
        capability = AllCapabilitites;
        contactQuery = term.mid(QString("log").length()).trimmed();
    } else {
        defaultAction = action(QLatin1String("start-text-chat"));
        capability = AllCapabilitites;
        contactQuery = term;
    }

    Q_FOREACH (const Tp::AccountPtr &account, m_accountManager->allAccounts()) {

        if (account->connection().isNull() || account->connection()->contactManager()->state() != Tp::ContactListStateSuccess) {
            continue;
        }

        Q_FOREACH (const Tp::ContactPtr &contact, account->connection()->contactManager()->allKnownContacts()) {

            Plasma::QueryMatch match(this);
            qreal relevance = 0.1;

            if (!hasCapability(contact, capability)) {
                continue;
            }

            const QString &normalized = contact->alias().normalized(QString::NormalizationForm_D);
            QString t;
            // Strip diacritics, umlauts etc
            Q_FOREACH (const QChar &c, normalized) {
                if (c.category() != QChar::Mark_NonSpacing
                        && c.category() != QChar::Mark_SpacingCombining
                        && c.category() != QChar::Mark_Enclosing) {
                    t.append(c);
                }
            }

            if (!t.contains(contactQuery, Qt::CaseInsensitive)) {
                //strip everything after the @, this is too avoid matching all '@facebook' addresses
                //when typing the word 'book'
                //if no @ symbol the entire string is searched
                const QString &id = contact->id().left(contact->id().indexOf(QLatin1Char('@')));
                if (!id.contains(contactQuery, Qt::CaseInsensitive)) {
                    continue;
                }
            }

            MatchInfo data;
            data.account = account;
            data.contact = contact;
            match.setData(qVariantFromValue(data));

            match.setText(contact->alias() + QLatin1String(" (") +  account->displayName() + ')');
            match.setType(Plasma::QueryMatch::ExactMatch);

            KTp::Presence presence(contact->presence());
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

            QString iconFile = contact->avatarData().fileName;
            if (!iconFile.isEmpty() && QFile::exists(iconFile)) {
                match.setIcon(QIcon(iconFile));
            } else {
                match.setIcon(QIcon::fromTheme(QLatin1String("im-user")));
            }

            if (!presence.statusMessage().isEmpty()) {
                match.setSubtext(presence.displayString() + QLatin1String(" | ") + presence.statusMessage());
            } else {
                match.setSubtext(presence.displayString());
            }

            match.setSelectedAction(defaultAction);
            match.setRelevance(relevance);

            context.addMatch(match);
        }
    }
}

void ContactRunner::matchPresence(Plasma::RunnerContext &context)
{
    const QString imKeyword = i18nc("A keyword to change IM status", "im");
    const QString statusKeyword = i18nc("A keyword to change IM status", "status");
    const QString connectCommand = i18nc("A command to connect all IM accounts", "connect");
    const QString disconnectCommand = i18nc("A command to disconnect all IM accounts", "disconnect");

    QString term = context.query().trimmed();

    if (!term.startsWith(imKeyword) && !term.startsWith(statusKeyword) &&
        (term != connectCommand) && (term != disconnectCommand)) {
        return;
    }

    /* Display all available presences? */
    bool all = ((term == imKeyword) || (term == statusKeyword));
    QString presenceString;
    QString statusMessage;

    if (!all) {
        /* Get string after the keyword */
        QString cmd = term.mid(term.indexOf(' ')).trimmed();

        int pos = cmd.indexOf(' ');
        if (pos > 0) {
            presenceString = cmd.mid(0, pos);
            statusMessage = cmd.mid(pos + 1);
        } else {
            presenceString = cmd;
        }
    }

    if (all || i18nc("IM presence", "online").contains(presenceString, Qt::CaseInsensitive) ||  (term == connectCommand)) {
        addPresenceMatch(context, Tp::ConnectionPresenceTypeAvailable, statusMessage);
    }

    if (all || i18nc("IM presence", "away").contains(presenceString, Qt::CaseInsensitive)) {
        addPresenceMatch(context, Tp::ConnectionPresenceTypeAway, statusMessage);
    }

    if (all || i18nc("IM presence","busy").contains(presenceString, Qt::CaseInsensitive)) {
        addPresenceMatch(context, Tp::ConnectionPresenceTypeBusy, statusMessage);
    }

    if (all || i18nc("IM presence","hidden").contains(presenceString, Qt::CaseInsensitive)) {
        addPresenceMatch(context, Tp::ConnectionPresenceTypeHidden, statusMessage);
    }

    if (all || i18nc("IM presence","offline").contains(presenceString, Qt::CaseInsensitive) || (term == disconnectCommand)) {
        addPresenceMatch(context, Tp::ConnectionPresenceTypeOffline, statusMessage);
    }
}

void ContactRunner::addPresenceMatch(Plasma::RunnerContext &context, Tp::ConnectionPresenceType presence,
                                     const QString &statusMessage)
{
    Plasma::QueryMatch match(this);
    match.setType(Plasma::QueryMatch::ExactMatch);

    MatchInfo data;

    switch (presence) {
        case Tp::ConnectionPresenceTypeAvailable:
            data.presence = KTp::Presence(Tp::Presence::available());
            match.setIcon(data.presence.icon());
            match.setText(i18nc("Description of runner action", "Set IM status to online"));
            break;
        case Tp::ConnectionPresenceTypeAway:
            data.presence = KTp::Presence(Tp::Presence::away());
            match.setIcon(data.presence.icon());
            match.setText(i18nc("Description of runner action", "Set IM status to away"));
            break;
        case Tp::ConnectionPresenceTypeBusy:
            data.presence = KTp::Presence(Tp::Presence::busy());
            match.setIcon(data.presence.icon());
            match.setText(i18nc("Description of runner action", "Set IM status to busy"));
            break;
        case Tp::ConnectionPresenceTypeHidden:
            data.presence = KTp::Presence(Tp::Presence::hidden());
            match.setIcon(data.presence.icon());
            match.setText(i18nc("Description of runner action", "Set IM status to hidden"));
            break;
        case Tp::ConnectionPresenceTypeOffline:
            data.presence = KTp::Presence(Tp::Presence::offline());
            match.setIcon(data.presence.icon());
            match.setText(i18nc("Description of runner action", "Set IM status to offline"));
            break;
        default:
            return;
    }

    if (!statusMessage.isEmpty()) {
        match.setSubtext(i18n("Status message: %1", statusMessage));
        data.presence.setStatusMessage(statusMessage);
    }

    match.setData(qVariantFromValue(data));

    context.addMatch(match);
}

K_EXPORT_PLASMA_RUNNER(ktp_contacts, ContactRunner)

#include "contactrunner.moc"
