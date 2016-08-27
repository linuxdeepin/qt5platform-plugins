#include "vtablehook.h"

QHash<qintptr**, qintptr*> VtableHook::objToOriginalVfptr;
QHash<void*, qintptr*> VtableHook::objToGhostVfptr;

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

    //! save original vfptr
    objToOriginalVfptr[obj] = *obj;
    *obj = new_vtable;
    //! save ghost vfptr
    objToGhostVfptr[obj] = new_vtable;

    return true;
}

bool VtableHook::clearGhostVtable(void *obj)
{
    qintptr *vtable = objToGhostVfptr.take(obj);

    if (vtable) {
        objToOriginalVfptr.remove((qintptr**)obj);

        delete[] vtable;

        return true;
    }

    return false;
}
