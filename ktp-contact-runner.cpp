#include "ktp-contact-runner.h"

#include <KDebug>
#include <KIcon>

ktp-contact-runner::ktp-contact-runner(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args);
    setObjectName("ktp-contact-runner");
}

ktp-contact-runner::~ktp-contact-runner()
{
}


void ktp-contact-runner::match(Plasma::RunnerContext &context)
{

    const QString term = context.query();
    if (term.length() < 3) {
        return;
    }
    //TODO
}

void ktp-contact-runner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
}

#include "ktp-contact-runner.moc"
