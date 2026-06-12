#include "solo/Solo.h"
#include <mimalloc.h>

int soloMain()
{
    {
        solo::S_Application app( 1280, 720 );
        app.exec(false);
    }
    mi_stats_print(nullptr);
    return 0;
}
