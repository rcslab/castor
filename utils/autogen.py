#!/usr/bin/env python
import sys

def parse_proto(line):
    print "proto:" + line
    half = line.index('(')
    prefix = line[:half]
    print "prefix:" + prefix
    result_type, name = prefix.split()
    print "result_type:" + result_type
    print "name:" + name
    suffix = line[half:]
    print "suffix:" + suffix
    argstring = suffix.strip('()')
    print argstring
    args = argstring.split(',')
    print "args: ", args
    argnames = map(lambda x: x.split()[-1].strip('*'), args)
    print "arg names:", argnames
    argtypes = map(lambda x: x.split()[:-1], args)
    print "arg types:", argtypes
    if result_type == 'int':
        result_type_string = 'I'
    elif result_type == 'ssize_t':
        result_type_string = 'S'
    else:
        print "ERROR: unknown result type"
        sys.exit(1)

    if argtypes[0][0] == 'int':
        leading_object = True
        print "leading object:", argnames[0]
        record_object_string = "O"
    else:
        record_object_string = ""
        leading_object = False

    record_method = "RRRecord%s%s" % (record_object_string, result_type_string)
    replay_method = "RRReplay%s%s" % (record_object_string, result_type_string)

    print "\n\n"
    print "%s" % result_type
    print "__rr_%s(%s)" % (name, argstring)
    print "{"
    print "\t%s result;\n" % result_type
    print "    switch (rrMode) {"
    print "\tcase RRMODE_NORMAL:"
    call_args = ["SYS_" + name] + argnames
    call_str =  "syscall(%s)" % ", ".join(call_args)
    print "\t    return %s;"  % call_str
    print "\tcase RRMODE_RECORD:"
    print "\t    result = %s;" % call_str
    rrevent = "RREVENT_%s" % name.upper()
    if leading_object:
        print "\t    %s(%s, %s, result);" % (record_method, rrevent, argnames[0])
    else:
        print "\t    %s(%s, result);" % (record_method, rrevent)
    print "\t    break;"
    print "\tcase RRMODE_REPLAY:"
    if leading_object:
        print "\t    %s(%s, %s, &result);" % (replay_method, rrevent, argnames[0])
    else:
        print "\t    %s(%s, &result);" % (replay_method, rrevent)
    print "\t    break;"
    print "    }"
    print "    return result;"
    print "}"

line = ''
while True:
    c = sys.stdin.read(1)
    if not c:
        break
    elif c == ';':
        parse_proto(line)
        line = ''
    else:
        if c == '\n':
            c = ' '
        line = line + c

