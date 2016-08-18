#include "vtablehook.h"

QHash<qintptr**, qintptr*> VtableHook::objToVfptr;

bool VtableHook::copyVtable(qintptr **obj)
{
    qintptr *vtable = *obj;

    while (*vtable > 0) {
        ++vtable;
    }

    int vtable_size = vtable - *obj;

    if (vtable_size == 0)
        return false;

    qintptr *new_vtable = new qintptr[++vtable_size];

    memcpy(new_vtable, *obj, (vtable_size) * sizeof(qintptr));

    objToVfptr[obj] = *obj;
    *obj = new_vtable;

    return true;
}
