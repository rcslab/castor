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
hout = open("event_gen.h", "w")

def c_output(line):
    print "cout", line
    line = line + '\n'
    cout.write(line)

def h_output(line):
    print "hout", line
    line = line + '\n'
    hout.write(line)

def gen_log_data(spec):
    gen_log = False
    result_cmp_str = {'@success_zero': 'result == 0', '@success_pos': 'result > 0',
                      '@success_nneg': 'result != -1'}[spec['success']]
    for arg in spec['args_spec']:
        if arg['log_spec'] != None:
            gen_log = True
    if gen_log:
        c_output("\t\tif (%s) {" % result_cmp_str)
        for arg in spec['args_spec']:
            if arg['log_spec']:
                c_output("\t\t\tlogData((uint8_t *)%s, %s);" %
                            (arg['name'], arg['log_spec']['size']))
        c_output("\t\t}")

def generate_handler(spec):
    print "handler_desc:", spec
    name = spec['name']
    result_type = spec['result_type']
    arg_list = [arg['type'] + ' ' + arg['name'] for arg in spec['args_spec']]
    arg_names = [arg['name'] for arg in spec['args_spec']]
    arg_types =  [arg['type'] for arg in spec['args_spec']]
    arg_string = ", ".join(arg_list)

    c_output("\n%s" % result_type)
    c_output("__rr_%s(%s)" % (name, arg_string))
    c_output("{")
    c_output("\t%s result;\n" % result_type)

    c_output("    switch (rrMode) {")
    c_output("\tcase RRMODE_NORMAL:")
    call_args = ["SYS_" + name] + arg_names
    call_str =  "syscall(%s)" % ", ".join(call_args)
    c_output("\t    return %s;"  % call_str)


    leading_object = True if arg_types[0][0] == 'int' else False
    object_string = 'O' if leading_object else ''
    result_type_string = {'int' : 'I', 'ssize_t' : 'S'}[result_type]
    rr_event = "RREVENT_%s" % name.upper()

    record_method = "RRRecord%s%s" % (object_string, result_type_string)
    c_output("\tcase RRMODE_RECORD:")
    c_output("\t    result = %s;" % call_str)
    if leading_object:
        c_output("\t    %s(%s, %s, result);" % (record_method, rr_event, arg_names[0]))
    else:
        c_output("\t    %s(%s, result);" % (record_method, rr_event))

    gen_log_data(spec)

    c_output("\t    break;")

    replay_method = "RRReplay%s%s" % (object_string, result_type_string)
    c_output("\tcase RRMODE_REPLAY:")
    if leading_object:
        c_output("\t    %s(%s, %s, &result);" % (replay_method, rr_event, arg_names[0]))
    else:
        c_output("\t    %s(%s, &result);" % (replay_method, rr_event))

    gen_log_data(spec)

    c_output("\t    break;")
    c_output("    }")
    c_output("    return result;")
    c_output("}")

def parse_logspec(sal, type):
    print "parse_logspec(%s, %s)" % (sal, type)
    log_spec = None
    sal = sal.strip()
    if sal.startswith("_Out_writes_bytes_("):
        size = sal.split('(')[1].strip(')')
        log_spec = { 'size': size}
    elif sal == "_Out_":
        size_type = type.split('*')[0].strip()
        log_spec = { 'size' : "sizeof(%s)" % size_type }
    return log_spec


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
        log_spec = None
        sal = None
        if fields[0].startswith("_"):
            sal = fields[0]
            del fields[0]
            arg_spec['sal'] = sal
            print "arg_sal:", arg_spec['sal']
        arg_spec['type'] = ' '.join(fields)
        print "arg_type:" + arg_spec['type']
        if sal != None:
            log_spec = parse_logspec(arg_spec['sal'], arg_spec['type'])
        arg_spec['log_spec'] = log_spec
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
    prefix_list = prefix.split()
    success = None
    if len(prefix_list) == 3:
        success = prefix_list[0]
        result_type = prefix_list[1]
        name = prefix_list[2]
    else:
        result_type = prefix_list[0]
        name = prefix_list[1]
    print "name:" + name
    print "result_type:" + result_type
    handler_spec['name'] = name
    handler_spec['result_type'] = result_type
    handler_spec['success'] = success
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

def generate_header():
    header = """
/*** this code has been autogenerated by code in utils/gen, do not modify it directly. ***/
"""
    c_output(header)

def generate_header_file(generated):
    index = 0x000A000
    h_output("#ifndef __EVENTS_GEN_H")
    h_output("#define __EVENTS_GEN_H\n\n")
    h_output("#define RREVENT_TABLE\\")
    last = generated.pop()
    for name in generated:
        h_output("\tRREVENT(%s,\t0x%X)\\" % (name.upper(), index))
        index += 1
    h_output("\tRREVENT(%s,\t0x%X)" % (last.upper(), index))

    h_output("#endif")



def generate_includes():
    includes = """
#include "util.h"
"""
    c_output(includes)

def generate_bindings(generated):
    c_output("")
    for name in generated:
        c_output("BIND_REF(%s);" % name)

if __name__ == '__main__':
    generated = []
    with open('autogenerate_list') as f:
            autogenerate_list = f.read().splitlines()
    generate_header()
    handler_description_list = parse_spec()
    generate_includes()
    print "desc:", handler_description_list
    print "list:", autogenerate_list
    for desc in handler_description_list:
        if desc['name'] in autogenerate_list:
            generate_handler(desc)
            generated.append(desc['name'])
    generate_bindings(generated)
    generate_header_file(generated)
    missing = set(autogenerate_list) - set(generated)
    if len(missing) != 0:
        sys.exit("Failed to generate: %s " % str(list(missing)))
    cout.close()
