#! /bin/env python

import cog
from string import Template

################################################################################
# .h template
################################################################################

contents = '''\
#ifndef $guard
#define $guard

#include "$dir/$base.h"

namespace $root {

class TypeFactory;

namespace $nested {

/** protobuf -> $dir/$clazz.proto
$protobuf_decl
**/
class $clazz : public $base {

    typedef $clazz Self;
    typedef $base Base;

    $clazz($ctor_params) : $ctor_inits {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    $member_decls
};

} // namespace $nested
} // namespace $root

#endif // $guard

'''

contents = Template(contents)

################################################################################
# .proto template
################################################################################

protobuf = '''\
import "$dir/$base.proto";

package $root.proto.$nested;

message $clazz {
    extend $root.proto.$base {
        optional $clazz obj = $tag;
    }
    $field_decls
}
'''

protobuf = Template(protobuf)

################################################################################
# Helpers
################################################################################

def type_decl(type, byValue):
    if byValue:
        return type
    else:
        return 'const {0}&'.format(type)

def ctor_param(field):
    (type, _, name, byValue) = field
    return '{0} {1}'.format(type_decl(type, byValue), name)

def base_ctor_init(base):
    return '{0}(class_tag(*this))'.format(base)

def ctor_init(field):
    (type, _, name, byValue) = field
    return '{0}({0})'.format(name)

def member_decl(field):
    (type, _, name, byValue) = field
    return '''
private:
    {0} {1};
public:
    {3} get{2}() const {{ return {1}; }}
'''.format(type, name, name.capitalize(), type_decl(type, byValue))

def field_decl(field, tag):
    (_, type, name, _) = field
    return 'optional {0} {1} = {2};'.format(type, name, tag)

################################################################################
# Filename: (
#   directory,
#   base class,
#   root namespace,
#   nested namespace,
#   class,
#   protobuf tag,
#   fields
# )
################################################################################

types = { \
"Integer.h"     :("Type", "Type", "borealis", "type", "Integer",     1,
                    []
                 ),
"Bool.h"        :("Type", "Type", "borealis", "type", "Bool",        2,
                    []
                 ),
"Float.h"       :("Type", "Type", "borealis", "type", "Float",       3,
                    []
                 ),
"Pointer.h"     :("Type", "Type", "borealis", "type", "Pointer",     4,
                    [("Type::Ptr", "borealis.proto.Type", "pointed", True)]
                 ),
"UnknownType.h" :("Type", "Type", "borealis", "type", "UnknownType", 5,
                    []
                 ),
"TypeError.h"   :("Type", "Type", "borealis", "type", "TypeError",   6,
                    [("std::string", "string", "message", False)]
                 ),
}

################################################################################
# Entry point
################################################################################

def spawn(filename):
    if filename.endswith('.h'):
        (dir, base, root, nested, clazz, tag, params) = types[filename]
        
        field_decls = '\n'.join([field_decl(param, idx+1) for idx, param in enumerate(params)])
        
        protobuf_decl = protobuf.substitute( \
            dir = dir, \
            root = root, \
            nested = nested, \
            base = base, \
            clazz = clazz, \
            tag = tag, \
            field_decls = field_decls \
        )
        
        ctor_params  = ', '.join([ctor_param(param) for param in params])
        ctor_inits   = ', '.join([base_ctor_init(base)] + [ctor_init(param) for param in params])
        member_decls = '\n'.join([member_decl(param) for param in params])
        
        cog.outl(contents.substitute( \
            guard = clazz.upper() + '_H', \
            dir = dir, \
            root = root, \
            nested = nested, \
            base = base, \
            clazz = clazz, \
            ctor_params = ctor_params, \
            ctor_inits = ctor_inits, \
            member_decls = member_decls, \
            protobuf_decl = protobuf_decl \
        ))
    elif filename.endswith('.cpp'):
    	cog.outl('#include "Type/{0}"'.format(filename.replace('.cpp', '.h')))
