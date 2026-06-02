#include "solo/Solo.h"
#include <mimalloc.h>

int soloMain()
{
    {
        S_Application app( 320, 240 );
        app.exec(false);
    }
    mi_stats_print(nullptr);
    return 0;
}
