#!/usr/bin/awk -f

BEGIN {
    IS_DECL=0
}

END {
    if (DATATYPE) {
        print "const char " OBJNAME "_EDS_TYPEDEF_NAME[] = \"" DATATYPE "\";\n"
    }
    else {
        print "extract_object: Error: No instantiation of " OBJNAME " found in source file" > "/dev/stderr";
        exit 1;
    }

    print "\n"
    print "void " OBJNAME "_GENERATOR(void *arg)"
    print "{\n"
    print "#include \"" SRC  "\"\n"
    print "EdsTableTool_DoExport(arg, &CFE_TBL_FileDef, &" OBJNAME ", sizeof(" OBJNAME "));"
    print "}"
}

# This preserves the original "CFE_TBL_FILEDEF" info
IS_DECL<0 {
    print
}

IS_DECL==0 {
    for (i=1; i < NF; i++) {
        if (match($i, "^" OBJNAME "\\>" ) != 0) {
            IS_DECL=i
            break
        }
    }

    if (IS_DECL) {
        for (i=1; i < IS_DECL; i++) {
            if ($i != "const") {
                DATATYPE=$i
            }
        }
        print DATATYPE
        print $IS_DECL
    }
    else {
        print $0
    }
}

IS_DECL > 0 && /;/ {
    IS_DECL=-1
    print ";"
}
