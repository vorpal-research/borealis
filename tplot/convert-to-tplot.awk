#!/bin/awk -f

{ print $1 " " $2 " " $3 get_trace_name(); }

function get_trace_name() {
    for (i = 4; i <= NF; i++) {
        if (match($i, ".*\\(", res)) {
            return substr(res[0], 0, length(res[0]) - 1);
        }
    }
    return $4;
}
