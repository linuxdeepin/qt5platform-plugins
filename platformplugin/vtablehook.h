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

#ifndef VTABLEHOOK_H
#define VTABLEHOOK_H

#include <QObject>
#include <QSet>
#include <QDebug>

#include "global.h"

DPP_BEGIN_NAMESPACE

static inline quintptr toQuintptr(void *ptr)
{
    return *(quintptr*)ptr;
}

template <typename ReturnType>
struct _TMP
{
public:
    template <typename Fun, typename... Args>
    static ReturnType callOriginalFun(const QHash<quintptr**, quintptr*> &objToOriginalVfptr,
                                      typename QtPrivate::FunctionPointer<Fun>::Object *obj, Fun fun, Args&&... args)
    {
        quintptr *vfptr_t2 = objToOriginalVfptr.value((quintptr**)obj, 0);

        if (!vfptr_t2)
            return (obj->*fun)(std::forward<Args>(args)...);

        quintptr fun1_offset = toQuintptr(&fun);

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return (obj->*fun)(std::forward<Args>(args)...);

        quintptr *vfptr_t1 = *(quintptr**)obj;
        quintptr old_fun = *(vfptr_t1 + fun1_offset / sizeof(quintptr));
        quintptr new_fun = *(vfptr_t2 + fun1_offset / sizeof(quintptr));

        // reset to original fun
        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = new_fun;

        // call
        const ReturnType &return_value = (obj->*fun)(std::forward<Args>(args)...);

        // reset to old_fun
        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = old_fun;

        return return_value;
    }
};
template <>
struct _TMP<void>
{
public:
    template <typename Fun, typename... Args>
    static void callOriginalFun(const QHash<quintptr**, quintptr*> &objToOriginalVfptr,
                                typename QtPrivate::FunctionPointer<Fun>::Object *obj, Fun fun, Args&&... args)
    {
        quintptr *vfptr_t2 = objToOriginalVfptr.value((quintptr**)obj, 0);

        if (!vfptr_t2)
            return (obj->*fun)(std::forward<Args>(args)...);

        quintptr fun1_offset = toQuintptr(&fun);

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return (obj->*fun)(std::forward<Args>(args)...);

        quintptr *vfptr_t1 = *(quintptr**)obj;
        quintptr old_fun = *(vfptr_t1 + fun1_offset / sizeof(quintptr));
        quintptr new_fun = *(vfptr_t2 + fun1_offset / sizeof(quintptr));

        // reset to original fun
        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = new_fun;

        // call
        (obj->*fun)(std::forward<Args>(args)...);

        // reset to old_fun
        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = old_fun;
    }
};

class VtableHook
{
public:
    //###(zccrs): 由于无法通过代码直接获取析构函数在虚表中的位置
    //            暂定QObject类型对象为第5项, 其它为第2项
    enum DestructFunIndex {
        Normal = 1,
        Qt_Object = 4
    };

    static constexpr DestructFunIndex getDestructFunIndex(...) { return Normal;}
    static constexpr DestructFunIndex getDestructFunIndex(const QObject*) { return Qt_Object;}
    static void autoCleanVtable(void *obj);

    template<typename T>
    static bool ensureVtable(T *obj) {
        quintptr **_obj = (quintptr**)(obj);

        if (objToOriginalVfptr.contains(_obj)) {
            // 不知道什么原因, 此时obj对象的虚表已经被还原
            if (objToGhostVfptr.value((void*)obj) != *_obj) {
                clearGhostVtable((void*)obj);
            } else {
                return true;
            }
        }

        if (!copyVtable(_obj))
            return false;

        // 备份对象的析构函数
        DestructFunIndex index = getDestructFunIndex(obj);
        quintptr *new_vtable = *((quintptr**)obj);
        objDestructFun[(void*)obj] = new_vtable[index];

        // 覆盖对象的析构函数, 用于自动清理虚表数据
        static bool testResult = false;
        class TestClass {
        public:
            static void testClean(T *that) {
                Q_UNUSED(that)
                testResult = true;
            }
        };

        // 尝试覆盖析构函数并测试
        testResult = false;
        new_vtable[index] = reinterpret_cast<quintptr>(&TestClass::testClean);
        delete obj;

        // 析构函数覆盖失败
        if (!testResult) {
            qFatal("Failed do override destruct function");
            qDebug() << "object:" << obj;
            abort();
        }

        // 真正覆盖析构函数
        new_vtable[index] = reinterpret_cast<quintptr>(&autoCleanVtable);

        return true;
    }

    template <typename T> class OverrideDestruct : public T { ~OverrideDestruct() override;};
    template <typename List1, typename List2> struct CheckCompatibleArguments { enum { value = false }; };
    template <typename List> struct CheckCompatibleArguments<List, List> { enum { value = true }; };
    template<typename Fun1, typename Fun2>
    static bool overrideVfptrFun(const typename QtPrivate::FunctionPointer<Fun1>::Object *t1, Fun1 fun1,
                      const typename QtPrivate::FunctionPointer<Fun2>::Object *t2, Fun2 fun2)
    {
        typedef QtPrivate::FunctionPointer<Fun1> FunInfo1;
        typedef QtPrivate::FunctionPointer<Fun2> FunInfo2;
        // 检查析构函数是否为虚
        class OverrideDestruct : public FunInfo1::Object { ~OverrideDestruct() override;};

        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X((CheckCompatibleArguments<typename FunInfo1::Arguments, typename FunInfo2::Arguments>::value),
                          "Function1 and Function2 arguments are not compatible.");
        Q_STATIC_ASSERT_X((CheckCompatibleArguments<QtPrivate::List<typename FunInfo1::ReturnType>, QtPrivate::List<typename FunInfo2::ReturnType>>::value),
                          "Function1 and Function2 return type are not compatible..");

        //! ({code}) in the form of a code is to eliminate - Wstrict - aliasing build warnings
        quintptr fun1_offset = toQuintptr(&fun1);
        quintptr fun2_offset = toQuintptr(&fun2);

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return false;

        if (!ensureVtable(t1)) {
            return false;
        }

        quintptr *vfptr_t1 = *(quintptr**)t1;
        quintptr *vfptr_t2 = *(quintptr**)t2;

        if (fun2_offset > UINT_LEAST16_MAX)
            *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = fun2_offset;
        else
            *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = *(vfptr_t2 + fun2_offset / sizeof(quintptr));

        return true;
    }

    template<typename Func> struct FunctionPointer { };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...)>
    {
        typedef QtPrivate::List<Obj*, Args...> Arguments;
    };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const>
    {
        typedef QtPrivate::List<Obj*, Args...> Arguments;
    };
    template<typename Fun1, typename Fun2>
    static bool overrideVfptrFun(const typename QtPrivate::FunctionPointer<Fun1>::Object *t1, Fun1 fun1, Fun2 fun2)
    {
        typedef QtPrivate::FunctionPointer<Fun1> FunInfo1;
        typedef QtPrivate::FunctionPointer<Fun2> FunInfo2;
        // 检查析构函数是否为虚
        class OverrideDestruct : public FunInfo1::Object { ~OverrideDestruct() override;};

        Q_STATIC_ASSERT(!FunInfo2::IsPointerToMemberFunction);
        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X((CheckCompatibleArguments<typename FunctionPointer<Fun1>::Arguments, typename FunInfo2::Arguments>::value),
                          "Function1 and Function2 arguments are not compatible.");
        Q_STATIC_ASSERT_X((CheckCompatibleArguments<QtPrivate::List<typename FunInfo1::ReturnType>, QtPrivate::List<typename FunInfo2::ReturnType>>::value),
                          "Function1 and Function2 return type are not compatible..");

        //! ({code}) in the form of a code is to eliminate - Wstrict - aliasing build warnings
        quintptr fun1_offset = toQuintptr(&fun1);
        quintptr fun2_offset = toQuintptr(&fun2);

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return false;

        if (!ensureVtable(t1)) {
            return false;
        }

        quintptr *vfptr_t1 = *(quintptr**)t1;
        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = fun2_offset;

        return true;
    }

    template<typename Fun1>
    static bool resetVfptrFun(const typename QtPrivate::FunctionPointer<Fun1>::Object *t1, Fun1 fun1)
    {
        quintptr *vfptr_t2 = objToOriginalVfptr.value((quintptr**)t1, 0);

        if (!vfptr_t2)
            return false;

        quintptr fun1_offset = toQuintptr(&fun1);

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return false;

        quintptr *vfptr_t1 = *(quintptr**)t1;

        *(vfptr_t1 + fun1_offset / sizeof(quintptr)) = *(vfptr_t2 + fun1_offset / sizeof(quintptr));

        return true;
    }

    template<typename Fun>
    static Fun originalFun(const typename QtPrivate::FunctionPointer<Fun>::Object *obj, Fun fun)
    {
        quintptr *vfptr_t2 = objToOriginalVfptr.value((quintptr**)obj, 0);

        if (!vfptr_t2)
            return fun;

        quintptr fun1_offset = *(quintptr *)&fun;

        if (fun1_offset < 0 || fun1_offset > UINT_LEAST16_MAX)
            return fun;

        quintptr *o_fun = vfptr_t2 + fun1_offset / sizeof(quintptr);

        return *reinterpret_cast<Fun*>(o_fun);
    }

    template<typename Fun, typename... Args>
    static typename QtPrivate::FunctionPointer<Fun>::ReturnType
    callOriginalFun(typename QtPrivate::FunctionPointer<Fun>::Object *obj, Fun fun, Args&&... args)
    {
        return _TMP<typename QtPrivate::FunctionPointer<Fun>::ReturnType>::callOriginalFun(objToOriginalVfptr, obj, fun, std::forward<Args>(args)...);
    }

private:
    static bool copyVtable(quintptr **obj);
    static bool clearGhostVtable(void *obj);

    static QHash<quintptr**, quintptr*> objToOriginalVfptr;
    static QHash<void*, quintptr*> objToGhostVfptr;
    static QMap<void*, quintptr> objDestructFun;
};

DPP_END_NAMESPACE

#endif // VTABLEHOOK_H
