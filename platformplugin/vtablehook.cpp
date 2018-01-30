/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vtablehook.h"

DPP_BEGIN_NAMESPACE

QHash<quintptr**, quintptr*> VtableHook::objToOriginalVfptr;
QHash<void*, quintptr*> VtableHook::objToGhostVfptr;
QMap<void*, quintptr> VtableHook::objDestructFun;

bool VtableHook::copyVtable(quintptr **obj, DestructFunIndex index)
{
    quintptr *vtable = *obj;

    while (*vtable > 0) {
        ++vtable;
    }

    int vtable_size = vtable - *obj;

    if (vtable_size == 0)
        return false;

    quintptr *new_vtable = new quintptr[++vtable_size];

    memcpy(new_vtable, *obj, (vtable_size) * sizeof(quintptr));

    //! save original vfptr
    objToOriginalVfptr[obj] = *obj;
    *obj = new_vtable;
    //! save ghost vfptr
    objToGhostVfptr[obj] = new_vtable;

    // 备份对象的析构函数
    objDestructFun[obj] = new_vtable[index];
    // 覆盖对象的析构函数, 用于自动清理虚表数据
    //TODE(zccrs): 加入尝试析构此对象的测试代码, 如果析构时没有调用autoCleanVtable
    //             则证明index为错误值, 此时应退出程序, 并更改代码添加此类型对应的DestructFunIndex枚举
    new_vtable[index] = reinterpret_cast<quintptr>(&autoCleanVtable);

    return true;
}

bool VtableHook::clearGhostVtable(void *obj)
{
    quintptr *vtable = objToGhostVfptr.take(obj);

    if (vtable) {
        quintptr **obj_ptr = (quintptr**)obj;
        objToOriginalVfptr.remove(obj_ptr);
        objDestructFun.remove(obj);

        delete[] vtable;

        return true;
    }

    return false;
}

void VtableHook::autoCleanVtable(void *obj)
{
    quintptr fun = objDestructFun.value(obj);

    if (!fun)
        return;

    typedef void(*Destruct)(void*);
    Destruct destruct = *reinterpret_cast<Destruct*>(&fun);
    // call origin destruct function
    destruct(obj);

    // clean
    clearGhostVtable(obj);
}

DPP_END_NAMESPACE
