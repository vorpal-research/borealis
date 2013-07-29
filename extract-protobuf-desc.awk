#!/bin/awk -f

BEGIN { PROTOBUF_DESC=0; }

/^\/\*\*\s*protobuf\s*->/ {
    PROTOBUF_DESC=$4;
    printf "" > PROTOBUF_DESC;
    next;
}

/^\*\*\// {
    PROTOBUF_DESC=0;
    next;
}

PROTOBUF_DESC != 0 {
    print >> PROTOBUF_DESC;
}
