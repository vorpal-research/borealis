#!/bin/awk -f

BEGIN {
    CURRENT_MODULE="NONE";
}

/^\[module\]/ {
    CURRENT_MODULE = gensub(/,/, "", 1, $2);
}

/^\s{4}/ {
    key = $1
    value = gensub(/.*default:\s(.*)\)/, "\\1", 1);
    type = gensub(/.*(\([^\(]*\)).*\(.*\)/, "\\1", 1);
    if ("NONE" == CURRENT_MODULE) {
        print key " = " value " " type;
    } else {
        print CURRENT_MODULE "." key " = " value " " type;
    }
}

function trimLast(s) {
    return substr(s, 0, length(s)-1)
}
