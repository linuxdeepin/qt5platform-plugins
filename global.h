#ifndef GLOBAL_H
#define GLOBAL_H

#define MOUSE_MARGINS 10

#define PUBLIC_CLASS(Class, Target) \
    class D##Class : public Class\
    {friend class Target;}

#define DEFINE_CONST_CHAR(Name) const char Name[] = #Name

DEFINE_CONST_CHAR(useDxcb);
DEFINE_CONST_CHAR(netWmStates);
DEFINE_CONST_CHAR(windowRadius);
DEFINE_CONST_CHAR(borderWidth);
DEFINE_CONST_CHAR(borderColor);
DEFINE_CONST_CHAR(shadowRadius);
DEFINE_CONST_CHAR(shadowOffset);
DEFINE_CONST_CHAR(shadowColor);
DEFINE_CONST_CHAR(clipPath);
DEFINE_CONST_CHAR(frameMask);

#endif // GLOBAL_H
