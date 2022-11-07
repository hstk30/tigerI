#include <stdio.h>

#include "temp.h"
#include "util.h"
#include "liveness.h"

static void
enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps)
{
    G_enter(t, flowNode, temps);
}

static Temp_tempList
lookupLiveMap(G_table t, G_node flowNode)
{
    return (Temp_tempList) G_look(t, flowNode);
}

