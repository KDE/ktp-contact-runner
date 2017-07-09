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

#ifndef KTPCONTACTRUNNER_H
#define KTPCONTACTRUNNER_H

#include <QModelIndex>
#include <QLoggingCategory>

#include <KRunner/AbstractRunner>

#include <TelepathyQt/Contact>

Q_DECLARE_LOGGING_CATEGORY(KTP_CONTACT_RUNNER)

namespace KTp
{
    class GlobalPresence;
    class PresenceModel;
    class AccountsListModel;
}

namespace Tp {
    class PendingOperation;
}

class QAction;

class ContactRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

  public:
    enum Capability {
        AllCapabilitites = 0,
        TextChatCapability,
        AudioCallCapability,
        VideoCallCapability,
        DesktopSharingCapability,
        FileTransferCapability
    };

    ContactRunner( QObject *parent, const QVariantList& args );
    ~ContactRunner();

    void match(Plasma::RunnerContext &context);
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

  protected:
    virtual void init();
    QList< QAction* > actionsForMatch(const Plasma::QueryMatch &match);

  private:
    bool hasCapability(const Tp::ContactPtr &contact, Capability capability) const;

    void matchPresence(Plasma::RunnerContext &context);
    void matchContacts(Plasma::RunnerContext &context);

    KTp::GlobalPresence *m_globalPresence;
    KTp::PresenceModel *m_presenceModel;
    KTp::AccountsListModel *m_accountsModel;

    bool m_loggerDisabled;
};

#endif /* KTPCONTACTRUNNER_H */
