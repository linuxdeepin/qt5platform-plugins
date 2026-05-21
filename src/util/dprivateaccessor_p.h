// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QtCore/qcompilerdetection.h>

// Private member accessor using the explicit template instantiation technique.
//
// C++ Standard [temp.explicit]/12 states:
// "The usual access checking rules do not apply to names used to
// specify explicit instantiation definitions."
//
// This allows passing pointers to private/protected data members and
// member functions as template arguments in explicit instantiations,
// bypassing normal access control — without modifying the class definition
// and without the UB caused by "#define private public".
//
// NOTE: These helper structs must be in the SAME namespace as the Tag structs
// (global namespace, since the macros expand at file scope). If they were in a
// sub-namespace, the friend definition would create a different function than
// the friend declaration in the Tag struct, causing undefined-reference errors.

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wnon-template-friend")

template<typename Tag>
struct Qt5PlatformPrivateAccessor
{
    using MemberPtr = typename Tag::MemberPtr;
    friend MemberPtr get(Tag) noexcept;
};

template<typename Tag, typename Tag::MemberPtr Ptr>
struct Qt5PlatformPrivateAccessorImpl : Qt5PlatformPrivateAccessor<Tag>
{
    friend typename Tag::MemberPtr get(Tag) noexcept { return Ptr; }
};

QT_WARNING_POP

#define D_DECLARE_PRIVATE_MEMBER(TagName, ClassName, Member, MemberType) \
    struct TagName { \
        using MemberPtr = MemberType ClassName::*; \
        friend MemberPtr get(TagName) noexcept; \
    }; \
    template struct Qt5PlatformPrivateAccessorImpl<TagName, &ClassName::Member>

#define D_DECLARE_PRIVATE_METHOD(TagName, ClassName, MethodName, RetType, ...) \
    struct TagName { \
        using MemberPtr = RetType (ClassName::*)(__VA_ARGS__); \
        friend MemberPtr get(TagName) noexcept; \
    }; \
    template struct Qt5PlatformPrivateAccessorImpl<TagName, &ClassName::MethodName>

#define D_DECLARE_PRIVATE_CONST_METHOD(TagName, ClassName, MethodName, RetType, ...) \
    struct TagName { \
        using MemberPtr = RetType (ClassName::*)(__VA_ARGS__) const; \
        friend MemberPtr get(TagName) noexcept; \
    }; \
    template struct Qt5PlatformPrivateAccessorImpl<TagName, &ClassName::MethodName>

// Trampoline: ensures get(tag) is called from a context with no class-scope
// get() member that might suppress ADL (C++ [basic.lookup.argdep] para 3).
namespace dtk_private_detail {
    template<typename Tag>
    inline typename Tag::MemberPtr access(Tag t) noexcept { return get(t); }

    // For auto-type tags (D_DECLARE_AUTO_PRIVATE_MEMBER_TAG) that don't define MemberPtr.
    template<typename Tag>
    inline auto access_auto(Tag t) noexcept -> decltype(get(t)) { return get(t); }
}

#define D_PRIVATE_MEMBER(obj, tag) ((obj).*dtk_private_detail::access(tag))
#define D_PRIVATE_CALL(obj, tag, ...) ((obj).*dtk_private_detail::access(tag))(__VA_ARGS__)

// For member types that involve private enum/type names that can't be spelled
// outside the class: use a C++17 auto non-type template parameter so the
// member pointer type is deduced without naming the private type explicitly.
template<auto Ptr>
struct Qt5PlatformAutoPrivateAccessor
{
    template<typename T>
    static decltype(auto) access(T &obj) noexcept { return obj.*Ptr; }
};

// Explicit instantiation to bypass access control at the instantiation point.
// After this, callers can reference Qt5PlatformAutoPrivateAccessor<Ptr>::access()
// without re-triggering access checks.
#define D_DECLARE_AUTO_PRIVATE_MEMBER(ClassName, Member) \
    template struct Qt5PlatformAutoPrivateAccessor<&ClassName::Member>

// Alternative auto accessor using the same get(Tag{}) pattern as D_DECLARE_PRIVATE_MEMBER,
// but without needing to name the member's type. Use this for members whose type involves
// private nested types (e.g., private enums) that cannot be spelled at the call site.
//
// The friend declaration MUST be in TagName itself so ADL can find get() when
// called as get(TagName{}) -- ADL only searches TagName's associated classes, not TagName##Impl.
//
// Usage:
//   D_DECLARE_AUTO_PRIVATE_MEMBER_TAG(MyTag, ClassName, myMember);
//   auto &ref = obj.*get(MyTag{});   // no access check needed here
#define D_DECLARE_AUTO_PRIVATE_MEMBER_TAG(TagName, ClassName, Member) \
    struct TagName {}; \
    template<auto Ptr> \
    struct TagName##Impl { \
        friend auto get(TagName) noexcept { return Ptr; } \
    }; \
    template struct TagName##Impl<&ClassName::Member>

#define D_AUTO_PRIVATE_MEMBER(obj, tag) ((obj).*dtk_private_detail::access_auto(tag{}))
