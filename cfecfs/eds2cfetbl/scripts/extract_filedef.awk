#!/usr/bin/awk -f

BEGIN {
    RS="\n";
    ORS="\n";
    FS=" ";
    IS_FILEDEF_MACRO=0;   # set nonzero if the CFE_TBL_FILEDEF macro is found
    IS_FILEDEF_DIRECT=0;  # set nonzero if the CFE_TBL_FileDef_t is instantiated directly (discouraged)
    DIRECT_CONTENT="";
}

END {
    if (IS_FILEDEF_MACRO >= 0) {
        print "\nERROR: Unable to extract CFE_TBL_FILEDEF macro from: " SRC "\n" > "/dev/stderr"
        exit 1
    }
}

# The C preprocessor outputs lines starting with # that indicate which file it came from
# We are only interested in content that was _directly_ in the table file here.
/^#/ {
    LINE=$2;
    FILE=$3;
    next;
}

# Ignore anything that is not in the original source file
FILE!="\"" SRC "\"" {
    next;
}

# Filedef starts at the CFE_TBL_FILEDEF macro
IS_FILEDEF_MACRO==0 && /\<CFE_TBL_FILEDEF\>/ {
    IS_FILEDEF_MACRO=1
    IS_FILEDEF_DIRECT=-1
}

IS_FILEDEF_DIRECT==0 {
    if ($1 == "CFE_TBL_FileDef_t") {
        IS_FILEDEF_DIRECT=1
        IS_FILEDEF_MACRO=-1
    }
}

IS_FILEDEF_DIRECT==1 {
    DIRECT_CONTENT=DIRECT_CONTENT $0
    if ($0 ~ /;/) {
        if (match(DIRECT_CONTENT, /{(.*)}/, OUT1) == 0) {
            print "\nERROR: Unable to extract CFE_TBL_FILEDEF info from direct instatiation: " DIRECT "\n" > "/dev/stderr"
            exit 1
        }
        # now that the CFE_TBL_FileDef_t content is identified, it first needs to be split at commas
        split(OUT1[1], OUT2, ",")
        # Then the first item taken out (only the first for now)
        match(OUT2[1], /"(.*)"/, OUT3)
        print "CFE_TBL_FILEDEF(" OUT3[1] ")"
        IS_FILEDEF_DIRECT=-1
    }
}

# send the "CFE_TBL_FILEDEF" macro content to the output
IS_FILEDEF_MACRO==1 {
    print
    if ($0 ~ /\)/)
        IS_FILEDEF_MACRO=-1
}
