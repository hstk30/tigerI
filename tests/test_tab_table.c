#include<stdio.h>

#include "table.h"


void show(void *key, void *value) {
    printf("%s -> %s\n", key, value);
}

int main() {
    TAB_table t = TAB_empty();

    TAB_enter(t, "a", "1");
    TAB_enter(t, "b", "2");
    TAB_enter(t, "c", "3");
    TAB_enter(t, "d", "4");
    TAB_enter(t, "e", "5");
    TAB_enter(t, "f", "6");
    TAB_enter(t, "g", "7");
    TAB_enter(t, "h", "8");
    TAB_enter(t, "i", "9");
    TAB_enter(t, "j", "10");
    TAB_enter(t, "k", "11");
    TAB_enter(t, "l", "12");

    TAB_dump(t, show);

    return 1;
}

