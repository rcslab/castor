#!/usr/bin/env python
#XXX TODO
#XXX -- generate header file
#XXX -- move in a bunch of examples from events.c to make sure we support them.
#XXX -- incorporate into runtime.
#XXX -- migrate all handlers that can exploit this.
#XXX -- document

import sys
import re

cout = open("event_gen.c", "w")

def c_output(line):
    print "cout", line
    line = line + '\n'
    cout.write(line)

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

def generate_handler(handler_desc):
    print "handler_desc:", handler_desc

def parse_args(arg_string):
    args_spec = []
    print "parse_args(%s)" % (arg_string)
    args = arg_string.split(',')
    print "args:", args
    for arg in args:
        arg_spec = {}
        if arg == 'void':
            break
        arg = arg.replace('*', '* ')
        fields = arg.split()
        arg_spec['name'] = fields[-1]
        print "arg_name:" + arg_spec['name']
        del(fields[-1])
        if fields[0].startswith("_"):
            arg_spec['sal'] = fields[0]
            del fields[0]
            print "arg_sal:" + arg_spec['sal']
        arg_spec['type'] = ' '.join(fields)
        print "arg_type:" + arg_spec['type']
        args_spec.append(arg_spec)
    return args_spec

def parse_proto(line):
    handler_spec = {}
    print "parse_proto(%s)" % (line)
    pivot = line.index('(')
    prefix = line[:pivot]
    suffix = line[pivot:]
    print "prefix:" + prefix
    print "suffix:%s<eol>" % (suffix)
    result_type, name = prefix.split()
    print "name:" + name
    print "result_type:" + result_type
    handler_spec['name'] = name
    handler_spec['result_type'] = result_type
    arg_string = suffix.strip('(); ')
    handler_spec['args_spec'] = parse_args(arg_string)
    return handler_spec

def parse_spec_line(line):
    print "parse_spec_line(%s):" % (line)
    pivot = line.index('{')
    prefix = line[:pivot]
    proto = line[pivot:]
    print "prefix:" + prefix
    print "proto:" + proto
    number, name, type = prefix.split()
    if type in ['OBSOL', 'UNIMPL']:
        pass
    else:
        proto = proto.strip('{}')
        return parse_proto(proto)

def parse_preprocessor(line):
    print "preprocessor", line
    c_output(line)

def parse_spec_comment(line):
    print "spec_comment", line
    return

def parse_newline(line):
    print "<newline>"
    c_output('')

def parse_spec():
    line_number = 0
    partial_line = None
    handler_description_list = []
    for line in sys.stdin:
        print line_number, line

        line_number += 1
        line = line.strip()

        if line == '':
            parse_newline(line)
        elif line.startswith("#"):
            parse_preprocessor(line)
        elif line.startswith(";"):
            parse_spec_comment(line)
        else:
            if line[0].isdigit():
                partial_line = line
            elif line.endswith("\\"):
                partial_line = partial_line + line
            elif line.endswith('}'):
                partial_line = partial_line + line

            if line.endswith('}'):
                spec_line = partial_line.replace("\\","")
                partial_line = None
                print "spec_line: %s" % (spec_line)
                try:
                    handler_desc = parse_spec_line(spec_line)
                    if handler_desc:
                        handler_description_list.append(handler_desc)
                except:
                    print "Unknown parsing error on line %d" % line_number
                    raise
    return handler_description_list

if __name__ == '__main__':
    with open('autogenerate_list') as f:
            autogenerate_list = f.read().splitlines()
    handler_description_list = parse_spec()
    print "desc:", handler_description_list
    print "list:", autogenerate_list
    for desc in handler_description_list:
        if desc['name'] in autogenerate_list:
            generate_handler(desc)
    cout.close()
