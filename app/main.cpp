#include "solo/Solo.h"

int soloMain()
{
    {
        S_Application app( 320, 240 );
        app.exec(false);

        s_debug( "#################### Used ####################",
                 "\nTotal allocated items:", S_Allocator::singleton()->getTotalAllocatedItems(),
                 "\nTotal used pools:", S_Allocator::singleton()->getTotalUsedPools(),
                 "\nTotal allocated bytes:", S_Allocator::singleton()->getTotalAllocatedBytes() );
    }
    s_debug( "#################### Invocation ####################",
    /*"\nTotal allocate invoked:",*/ S_Allocator::singleton()->getTotalAllocateInvoked(),
    /*"\nTotal deallocate invoked:",*/ S_Allocator::singleton()->getTotalDeallocateInvoked() );
    return 0;
}
