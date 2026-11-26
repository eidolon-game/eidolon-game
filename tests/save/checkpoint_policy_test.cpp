#include "CheckpointPolicy.h"
#include <cassert>
#include <iostream>
using namespace EidolonCheckpoint;
int main()
{
    const Copy m{Kind::Missing}, c{Kind::Corrupt}, f{Kind::Future}, u{Kind::Unreadable};
    const Copy a{Kind::Valid,0}, b{Kind::Valid,1}, a2{Kind::Valid,2};
    assert(Next(m,m,-1).target == 0);
    assert(Next(a,m,0).target == 1);
    assert(Next(a,b,1).target == 0);
    assert(Select(c,b).newest == 1); // interrupted overwrite of older copy
    assert(Select(a,c).newest == 0);
    assert(Next(c,b,1).generation == 2);
    assert(Select(c,c).blocked && Next(c,c,-1).target == -1);
    assert(Select(f,b).blocked && Next(a,f,0).target == -1);
    assert(Select(a,u).blocked);
    assert(Next(a2,b,1).target == -1); // stale writer
    assert(Select({Kind::Valid,1},b).blocked);
    assert(Next(a,{Kind::Valid,INT64_MAX},INT64_MAX).target == -1);
    // Enumerate every interruption between plan and write completion.
    for (Copy outcome : {a,c,a2}) {
        auto s = Select(outcome,b);
        assert(!s.blocked);
        assert(s.newest == (outcome.generation == 2 ? 0 : 1));
    }
    std::cout << "PASS: checkpoint policy (12 assertions + 3 interruption cases)\n";
}
