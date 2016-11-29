#!/bin/awk -f

BEGIN {
    PROTOBUF_DESC=0;
    cMap[""]=-1;
}

/^\/\*\*\s*protobuf\s*->/ {
    PROTOBUF_DESC=$4;
    print "syntax = \"proto2\";" > PROTOBUF_DESC;
    next;
}

/^\*\*\// {
    PROTOBUF_DESC=0;
    next;
}

PROTOBUF_DESC != 0 {
    cRegex = "\\$COUNTER\\_(\\w+)";

    hasCounter = match($0, cRegex, m);
    if (hasCounter != 0) {
        cName = m[1];
        cValue = cMap[cName];
        if (cValue == "") cValue = 16;
        sub(cRegex, cValue, $0);
        cMap[cName] = cValue + 1;
    }
    
    print >> PROTOBUF_DESC;
}
