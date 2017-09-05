#!/usr/bin/env python
#XXX TODO
#XXX -- finish handler generator
#XXX -- generate header file
#XXX -- move in a bunch of examples from events.c to make sure we support them.
#XXX -- incorporate into runtime.
#XXX -- migrate all handlers that can exploit this.
#XXX -- document

import sys
import re

line_number = 0

cout = open("event_gen.c", "w")
hout = open("event_gen.h", "w")

def c_output(line):
    print "cout", line
    line = line + '\n'
    cout.write(line)

def h_output(line):
    line = line + '\n'
    hout.write(line)

def die(msg):
    print "Error on line %d:%s" % (line_number, msg)
    sys.exit(1)

def parse_dataspec(proto_spec, line):
    data_spec = {}
    _,specstr= line.split('$')
    print "specstr:", specstr
    specstr = specstr.strip()
    if (specstr[0] != '{'):
        die("missing opening brace")
    if (specstr[-1] != '}'):
        die("missing closing brace")
    specstr = specstr.strip('{}')
    parts = specstr.split('|')
    if (len(parts) != 2):
        die("missing portion of data spec, should be {result_str | arg_str}")
    result_str, arg_str = parts

    result_str = result_str.strip()
    valid_results = ['result>0', 'result==0', 'result!=-1']
    if not (result_str in valid_results):
        die("\'%s\' not in %s" % (result_str, str(valid_results)))
    else:
        data_spec['result_cmp'] = result_str

    arg_str = arg_str.strip()
    arg_ar = arg_str.split(',')
    log_args = {}
    for arg_spec in arg_ar:
        if len(arg_spec.split(':')) != 2:
            die("invalid arg spec, should be arg_name:arg_type")
        a_name, a_type = arg_spec.split(':')
        a_name = a_name.strip()
        a_type = a_type.strip()
        valid_names = proto_spec['arg_names']
        if not (a_name in valid_names):
            die("invalid argument name \'" + a_name + "\' not in " + str(valid_names))

        m = re.match('arg\(([a-zA-Z_0-9]+)\)',a_type)
        if m:
            if not (m.groups()[0] in proto_spec['arg_names']):
                die("invalid parameter to arg() \'" + m.groups()[0] + "\' parameter must be" +
                        " in formal arguments of " + proto_spec['name'])
            log_args[a_name] = m.groups()[0]
        elif a_type == 'sizeof':
            log_args[a_name] = 'sizeof(*%s)' % a_name

        else:
            die("invalid argument type \'" + a_type + "\'  must be either sizeof or arg(...)")

    data_spec['log_args'] = log_args
    return data_spec

def parse_proto(line):
    proto_spec = {}
    if '$' in line:
        line,_ = line.split('$')
    if not (';') in line:
        die('protocol line missing semi-colon')
    else:
        line,_ = line.split(';')
    half = line.index('(')
    prefix = line[:half]
    print "prefix:" + prefix
    result_type, name = prefix.split()
    print "result_type:" + result_type
    result_type = result_type.strip()
    result_types = ['int', 'ssize_t']
    if not (result_type in result_types):
        die("unknown result type \'" + result_type + "\' not in " + str(result_types))
    print "name:" + name
    proto_spec['name'] = name
    proto_spec['result_type'] = result_type

    suffix = line[half:]
    print "suffix:" + suffix
    arg_string = suffix.strip('()')
    print "arg_string:" + arg_string
    proto_spec['arg_string'] = arg_string
    args = arg_string.split(',')
    print "args: ", args
    argnames = map(lambda x: x.split()[-1].strip('*'), args)
    print "arg names:", argnames
    proto_spec['arg_names'] = argnames
    argtypes = map(lambda x: x.split()[:-1], args)
    print "arg types:", argtypes
    proto_spec['arg_types'] = argtypes
    return proto_spec

def gen_log_data(data_spec):
    #XXX: should remove redundancy with spec parser
    result_cmp_str = {'result==0': 'result == 0', 'result>0': 'result > 0', 'result!=-1': 'result != -1'}[data_spec['result_cmp']]
    c_output("    if (%s) {" % result_cmp_str)
    #XXX: add support for using other arguments for data size
    for name, size in data_spec['log_args'].iteritems():
        c_output("\t\tlogData((uint8_t *)%s, %s);" % (name, size))
    c_output("    }")


#XXX: fix formatting
def gen_handler(proto_spec, data_spec):
    result_type = proto_spec['result_type']
    name = proto_spec['name']
    arg_string = proto_spec['arg_string']
    arg_names = proto_spec['arg_names']
    arg_types = proto_spec['arg_types']
    leading_object = True if arg_types[0][0] == 'int' else False
    object_string = 'O' if leading_object else ''
    result_type_string = {'int' : 'I', 'ssize_t' : 'S'}[result_type]
    record_method = "RRRecord%s%s" % (object_string, result_type_string)
    replay_method = "RRReplay%s%s" % (object_string, result_type_string)
    rr_event = "RREVENT_%s" % name.upper()

    c_output("\n%s" % result_type)
    c_output("__rr_%s(%s)" % (name, arg_string))
    c_output("{")
    c_output("\t%s result;\n" % result_type)
    c_output("    switch (rrMode) {")

    c_output("\tcase RRMODE_NORMAL:")
    call_args = ["SYS_" + name] + arg_names
    call_str =  "syscall(%s)" % ", ".join(call_args)
    c_output("\t    return %s;"  % call_str)

    c_output("\tcase RRMODE_RECORD:")
    c_output("\t    result = %s;" % call_str)
    if leading_object:
        c_output("\t    %s(%s, %s, result);" % (record_method, rr_event, arg_names[0]))
    else:
        c_output("\t    %s(%s, result);" % (record_method, rr_event))

    if data_spec != None:
        gen_log_data(data_spec)

    c_output("\t    break;")

    c_output("\tcase RRMODE_REPLAY:")
    if leading_object:
        c_output("\t    %s(%s, %s, &result);" % (replay_method, rr_event, arg_names[0]))
    else:
        c_output("\t    %s(%s, &result);" % (replay_method, rr_event))

    if data_spec != None:
        gen_log_data(data_spec)

    c_output("\t    break;")
    c_output("    }")
    c_output("    return result;")
    c_output("}")


def parse_c_comment(line):
    print "c_comment", line
    c_output(line)

def parse_preprocessor(line):
    print "preprocessor", line
    c_output(line)

def parse_spec_comment(line):
    print "spec_comment", line
    return

def parse_newline(line):
    print "<newline>"
    c_output('')

for line in sys.stdin:
    print line_number, line
    line_number += 1
    line = line.strip()

    if line == '':
        parse_newline(line)
    elif line.startswith("#"):
        parse_preprocessor(line)
    elif line.startswith("//"):
        parse_c_comment(line)
    elif line.startswith("%"):
        parse_spec_comment(line)
    else:
        try:
            data_spec = None
            proto_spec = parse_proto(line)
            if "$" in line:
                data_spec = parse_dataspec(proto_spec, line)
            print "proto_spec", str(proto_spec)
            print "data_spec", str(data_spec)
            gen_handler(proto_spec, data_spec)
        except:
            print "Unknown parsing error on line %d" % line_number
            raise

cout.close()
hout.close()
