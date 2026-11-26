#include "../Source/Eidolon/World/EidolonWorldAccess.h"

#include <cassert>
#include <iostream>

int main()
{
    using namespace EidolonWorld;
    Progress State;
    assert(!CanEnter(Region::Valdris, State));
    State.Chapter = 1;
    assert(CanEnter(Region::Valdris, State));
    assert(!CanEnter(Region::GiantTerritory, State));
    State.Chapter = 4;
    State.AegisSeal = true;
    assert(!CanEnter(Region::GiantTerritory, State));
    State.Chapter = 5;
    assert(CanEnter(Region::GiantTerritory, State));
    assert(!CanEnter(Region::Continent, State));
    State.Chapter = 7;
    State.GiantsDefeated = 100;
    assert(!CanEnter(Region::Continent, State));
    State.Chapter = 8;
    assert(!CanEnter(Region::Continent, State));
    State.HorizonSeal = true;
    State.GiantsDefeated = 99;
    assert(!CanFastTravel(Region::Continent, State, true, true, false));
    State.GiantsDefeated = 100;
    assert(CanEnter(Region::Continent, State));
    assert(!CanFastTravel(Region::Continent, State, false, true, false));
    assert(!CanFastTravel(Region::Continent, State, true, false, false));
    assert(!CanFastTravel(Region::Continent, State, true, true, true));
    assert(CanFastTravel(Region::Continent, State, true, true, false));
    assert(!CanEnter(Region::Vareth, State));
    State.Chapter = 10;
    assert(CanEnter(Region::Vareth, State));
    State.Chapter = 19;
    assert(!CanEnter(Region::Vareth, State));
    assert(!CanEnter(Region::Oblivion, State));
    State.Chapter = 20;
    assert(CanEnter(Region::Oblivion, State));
    assert(!CanEnter(Region::Stair, State));
    State.Chapter = 23;
    assert(CanEnter(Region::Stair, State));
    State.StairFloor = 100;
    assert(!CanEnter(Region::Empyrea, State));
    State.Chapter = 24;
    assert(CanEnter(Region::Empyrea, State));
    State.StairFloor = 99;
    assert(!CanEnter(Region::Empyrea, State));
    std::cout << "World progression and fast-travel gates passed\n";
}
