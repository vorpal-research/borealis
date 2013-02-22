#! /bin/env python

import cog
from string import Template

contents = '''\
#ifndef $guard
#define $guard

#include "Type/Type.h"

namespace $namespace {

class $clazz : public $base {
    typedef $clazz self;
    typedef $base base;


    $clazz($member_params) : $base(type_id(*this))$member_cons {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class TypeFactory;

    $members
};

} // namespace $namespace

#endif // $guard

'''
contents = Template(contents)

def constructor_param(field):
    (type, name, byValue) = field
    if byValue:
        return '{0} {1}'.format(type, name)
    else:
        return 'const {0}& {1}'.format(type, name)

constructor_invoke = '{0}({0})'

def param_declare(field):
    (type, name, byValue) = field
    template = '''
private:
    {0} {1};
public:
    {3} get{2}() const {{ return {1}; }}
'''
    if byValue:
        return template.format(type, name, name.capitalize(), type)
    else:
        return template.format(type, name, name.capitalize(), 'const ' + type + '&')


# Filename: (directory, base class, namespace, class, fields)
types = { \
"Integer.h" :("Type", "Type", "borealis", "Integer", []),
"Bool.h" :("Type", "Type", "borealis", "Bool", []),
"Float.h" :("Type", "Type", "borealis", "Float", []),
"Pointer.h" :("Type", "Type", "borealis", "Pointer", [("Type::Ptr", "pointed", True)]),
"UnknownType.h" :("Type", "Type", "borealis", "UnknownType", []),
"TypeError.h" :("Type", "Type", "borealis", "TypeError", [("std::string", "message", False)]),
}

def spawn(filename):
    if filename.endswith('.h'):
        (dir, base, namespace, clazz, params) = types[filename]
        param_string = ','.join([constructor_param(param) for param in params])
        param_conses = ''.join([', ' + constructor_invoke.format(name) for (type, name, byValue) in params])
        member_decls = '\n'.join([param_declare(param) for param in params])
        cog.outl(contents.substitute( \
            dir = dir, \
            base = base, \
            namespace = namespace, \
            clazz = clazz, \
            member_params = param_string, \
            member_cons = param_conses, \
            members = member_decls, \
            guard = clazz.upper() + "_H" \
        ))
    elif filename.endswith('.cpp'):
    	cog.outl('#include "Type/{}"'.format(filename.replace('.cpp', '.h')))
