#include "../Source/Eidolon/Dungeons/EidolonCounterweightRules.h"

#include <iostream>

using namespace EidolonCounterweight;

int main()
{
    Progress Committed;
    const Event InitialCollection{Committed, Action::Collect, Evidence::Source};
    if (!IsValid(Committed) || CanExit(Committed)
        || Advance(Committed, {Committed, Action::Collect, Evidence::Winch})
        || Advance(Committed, {Committed, Action::Install, Evidence::Winch})
        || Advance(Committed, {Committed, Action::Recover, Evidence::CarrierRecovery}))
    {
        std::cerr << "Source allowed an invalid interaction\n";
        return 1;
    }

    const auto Carried = Advance(Committed, InitialCollection);
    if (!Carried || Carried->Weight != Location::Carried || Carried->Revision != 1u
        || CanExit(*Carried) || Advance(*Carried, InitialCollection))
    {
        std::cerr << "Collection did not produce one locked carried state\n";
        return 1;
    }
    Committed = *Carried;

    // The recovery callback represents a confirmed drop, respawn or invalid position.
    const Event Recovery{Committed, Action::Recover, Evidence::CarrierRecovery};
    if (Advance(Committed, {Committed, Action::Recover, Evidence::Source})
        || Advance(Committed, {Committed, Action::Install, Evidence::Source}))
    {
        std::cerr << "Carrier accepted recovery or installation from a wrong producer\n";
        return 1;
    }
    const auto Recovered = Advance(Committed, Recovery);
    if (!Recovered || Recovered->Weight != Location::AtSource || Recovered->Revision != 2u
        || CanExit(*Recovered) || Advance(*Recovered, Recovery) || Advance(*Recovered, InitialCollection))
    {
        std::cerr << "Recovery lost the source or allowed an old event after returning to it\n";
        return 1;
    }
    Committed = *Recovered;

    const auto Recollected = Advance(Committed, {Committed, Action::Collect, Evidence::Source});
    if (!Recollected || Recollected->Weight != Location::Carried || Recollected->Revision != 3u)
    {
        std::cerr << "A fresh collection was rejected after recovery\n";
        return 1;
    }
    Committed = *Recollected;

    const Event Install{Committed, Action::Install, Evidence::Winch};
    const auto Candidate = Advance(Committed, Install);
    // A simulated write failure leaves the live state carried and the exit shut.
    if (!Candidate || CanExit(Committed) || Candidate->Weight != Location::Installed
        || Candidate->Revision != 4u || !Advance(Committed, Install))
    {
        std::cerr << "Failed install could not be retried safely\n";
        return 1;
    }
    Committed = *Candidate;
    const Progress Reloaded = Committed;
    if (!CanExit(Reloaded) || Advance(Reloaded, Install)
        || Advance(Reloaded, {Reloaded, Action::Install, Evidence::Winch})
        || Advance(Reloaded, {Reloaded, Action::Collect, Evidence::Source})
        || Advance(Reloaded, {Reloaded, Action::Recover, Evidence::CarrierRecovery}))
    {
        std::cerr << "Installed checkpoint was not permanent or idempotent\n";
        return 1;
    }

    const Progress Exhausted{Location::Carried, std::numeric_limits<std::uint32_t>::max()};
    if (CanExit({Location::Installed, 0}) || IsValid({Location::AtSource, 1})
        || IsValid({Location::Carried, 2}) || IsValid({Location::Installed, 3})
        || IsValid({static_cast<Location>(255), 0})
        || Advance(Exhausted, {Exhausted, Action::Recover, Evidence::CarrierRecovery}))
    {
        std::cerr << "Invalid persisted state or revision overflow was accepted\n";
        return 1;
    }

    std::cout << "Counterweight: source, carry, recovery, replay, install and reload passed\n";
}
