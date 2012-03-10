
#ifndef KTP-CONTACT-RUNNER_H
#define KTP-CONTACT-RUNNER_H

#include <plasma/abstractrunner.h>
#include <KIcon>

// Define our plasma Runner
class ktp-contact-runner : public Plasma::AbstractRunner {
    Q_OBJECT

public:
    // Basic Create/Destroy
    ktp-contact-runner( QObject *parent, const QVariantList& args );
    ~ktp-contact-runner();

    void match(Plasma::RunnerContext &context);
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);
};
// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_RUNNER(ktp-contact-runner, ktp-contact-runner)

#endif
