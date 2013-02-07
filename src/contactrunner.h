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

#include <QtCore/QModelIndex>

#include <Plasma/AbstractRunner>
#include <KIcon>

#include <TelepathyQt/AccountManager>

namespace KTp
{
    class GlobalPresence;
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
    QList< QAction* > actionsForMatch(const Plasma::QueryMatch &match);

  private Q_SLOTS:
    void accountManagerReady(Tp::PendingOperation *operation);

  private:
    bool hasCapability(const Tp::ContactPtr &contact, Capability capability) const;

    void matchPresence(Plasma::RunnerContext &context);
    void matchContacts(Plasma::RunnerContext &context);

    void addPresenceMatch(Plasma::RunnerContext &context, Tp::ConnectionPresenceType presence,
                          const QString &statusMessage);

    KTp::GlobalPresence *m_globalPresence;
    Tp::AccountManagerPtr m_accountManager;

    bool m_loggerDisabled;
};

K_EXPORT_PLASMA_RUNNER(ktp_contacts, ContactRunner)

#endif /* KTPCONTACTRUNNER_H */
