#!/usr/bin/env python

import sys
import os
import re
import subprocess

HANDLER_PATH = "./events_gen.c"
HEADER_PATH = "./events_gen.h"

cout = open(HANDLER_PATH, "w")
hout = open(HEADER_PATH, "w")

verbose = False

def die(message):
    raise ValueError(message)

def debug(*args):
    if verbose:
        print "DEBUG>>  ".join(map(str,args))

def incomplete(*args):
    if verbose:
        print "INCOMPLETE ".join(map(str,args))


def c_output(line):
    if verbose:
        debug("cout", line)
    line = line + '\n'
    cout.write(line)

def h_output(line):
    if verbose:
        debug("hout", line)
    line = line + '\n'
    hout.write(line)

def gen_log_data(spec):
    gen_log = False
    for arg in spec['args_spec']:
        if arg['log_spec'] != None:
            gen_log = True
    if gen_log:
        c_output("\t\tif (result != -1) {")
        for arg in spec['args_spec']:
            if arg['log_spec'] != None:
                #XXX formatting ugly
                if arg['log_spec']['null_check']:
                    c_output("\t\t\t if (%s != NULL) {" % (arg['name']))
                c_output("\t\t\tlogData((uint8_t *)%s, %s);" %
                            (arg['name'], arg['log_spec']['size']))
                if arg['log_spec']['null_check']:
                    c_output("\t\t\t }")
        c_output("\t\t}")

def generate_handler(spec):
    debug("handler_desc:", spec)
    name = spec['name']
    result_type = spec['result_type']
    arg_list = [arg['type'] + ' ' + arg['name'] for arg in spec['args_spec']]
    arg_names = [arg['name'] for arg in spec['args_spec']]
    arg_types =  [arg['type'] for arg in spec['args_spec']]
    arg_string = ", ".join(arg_list)
    syscall_number = "SYS_" + name
    event_number = "RREVENT_%s" % name.upper()
    leading_object = arg_types and (arg_types[0] == 'int')
    call_args = [syscall_number] + arg_names
    syscall_str =  "syscall(%s)" % ", ".join(call_args)

    #handler opening
    c_output("\n%s" % result_type)
    c_output("__rr_%s(%s)" % (name, arg_string))
    c_output("{")
    c_output("%s result;" % result_type)
    c_output("RRLogEntry *e;\n")

    #c_output("DLOG(\"entering %s\");" % name)
    c_output("switch (rrMode) {")

    #normal mode
    c_output("case RRMODE_NORMAL:")
    c_output("return %s;"  % syscall_str)

    #record mode
    c_output("case RRMODE_RECORD:")
    c_output("result = %s;" % syscall_str)
    c_output("e = RRLog_Alloc(rrlog, threadId);")
    c_output("e->event = %s;" % event_number)
    if leading_object:
        c_output("e->objectId = (uint64_t)%s;" % arg_names[0])
    c_output("e->value[0] = (uint64_t)result;")
    c_output("if (result == -1) {")
    c_output("e->value[1] = (uint64_t)errno;")
    c_output("}")
    c_output("RRLog_Append(rrlog, e);")
    gen_log_data(spec)
    c_output("break;")

    #replay mode
    c_output("case RRMODE_REPLAY:")
    c_output("e = RRPlay_Dequeue(rrlog, threadId);")
    c_output("AssertEvent(e, %s);" % event_number)
    if leading_object:
        c_output("AssertObject(e, (uint64_t)%s);" % arg_names[0])
    c_output("result = (%s)e->value[0];" % result_type)
    c_output("if (result == -1) {")
    c_output("errno = e->value[1];")
    c_output("}")
    c_output("RRPlay_Free(rrlog, e);")
    gen_log_data(spec)
    c_output("\t    break;")

    c_output("    }") #end switch

    c_output("return result;")
    c_output("}")


def parse_logspec(sal, type):
    debug("parse_logspec(%s, %s)" % (str(sal), type))
    sal_tag = sal['tag']
    sal_arg = sal['arg']

    log_spec = None
    null_check = '_opt_' in sal_tag
    sal_tag = re.sub("opt_","",sal_tag)

    if sal_tag.startswith('_In_'):
       pass
    elif sal_tag == "_Out_writes_bytes_":
        log_spec = { 'size': sal_arg}
    elif sal_tag == "_Out_writes_z_":
        count = sal_arg
        base_type = type.split('*')[0].strip()
        log_spec = { 'size': "%s * sizeof(%s)" % (count, base_type) }
    elif sal_tag == "_Out_" or sal_tag == '_Inout_':
        size_type = type.split('*')[0].strip()
        log_spec = { 'size' : "sizeof(%s)" % size_type }
    else:
        debug("unknown sal:" + str(sal))

    if log_spec != None:
        log_spec['null_check'] = null_check

    return log_spec


def parse_args(arg_string):
    args_spec = []
    args = arg_string.split(',')
    debug("args:", args)
    for arg in args:
        arg_spec = {}
        if arg == 'void':
            break
        arg = arg.strip()

        #chop name
        so = re.search("(\w+$)", arg)
        name = so.group()
        arg = re.sub("\w+$", "", arg)
        arg_spec['name']  = name
        debug("arg_name:" + arg_spec['name'])

        #chop sal
        sal = None
        if arg.startswith("_In") or arg.startswith("_Out"):
            SAL_PATTERN = "^(?P<tag>(\_\w+\_))(\((?P<arg>[\w\*]+)\))?"
            so = re.search(SAL_PATTERN, arg)
            sal = {'tag': so.group('tag'), 'arg' :so.group('arg')}
            arg = re.sub(SAL_PATTERN,"",arg)
            arg_spec['sal'] = sal
            debug("arg_sal:", str(sal))

        #type is whats left
        arg_spec['type'] = arg.strip()

        debug("arg_type:" + arg_spec['type'])

        if sal != None:
            arg_spec['log_spec'] = parse_logspec(arg_spec['sal'], arg_spec['type'])
        else:
            arg_spec['log_spec'] = None

        args_spec = args_spec + [arg_spec]
    debug("args_spec: " + str(args_spec))
    return args_spec

def parse_proto(proto, handler_spec):
    debug("parse_proto(%s)" % (proto))
    proto = proto.strip()
    pivot = proto.index('(')
    prefix = proto[:pivot]
    suffix = proto[pivot:]
    debug("prefix:" + prefix)
    debug("suffix:%s<eol>" % (suffix))
    result_type, name = prefix.split()
    debug("name:" + name)
    debug("result_type:" + result_type)
    handler_spec['name'] = name
    handler_spec['result_type'] = result_type
    arg_string = suffix.strip('(); ')
    handler_spec['args_spec'] = parse_args(arg_string)
    return handler_spec

def parse_spec_line(line):
    debug("spec_line:" + line)
    handler_spec = {}
    pivot = line.index('{')
    prefix = line[:pivot]
    proto = line[pivot:]
    debug("prefix:" + prefix)
    debug("proto:" + proto)
    number, name, type = prefix.split()
    handler_spec['decl_type'] = type
    if not type in ['STD', 'COMPAT']:
        debug("===Unused line===")
    else:
        tail = proto.rfind('}')
        proto = proto[:tail]
        proto = proto.strip('{')
        return parse_proto(proto, handler_spec)

def parse_preprocessor(line):
    debug("preprocessor:", line)
    c_output(line)

def parse_spec_comment(line):
    debug("spec_comment:", line)
    return

def parse_newline(line):
    debug("<newline>:")
    c_output('')

def parse_unused(line):
    debug("unused:" + line)
    #XXX: add an assertion to check this

#STD spec_lines take precendence over COMPAT lines
def remove_duplicates(desc_list):
    for desc_a in desc_list:
        for desc_b in desc_list:
            if desc_a['name'] == desc_b['name'] and desc_a != desc_b:
                if desc_a['decl_type'] == 'STD' and desc_b['decl_type'] == 'COMPAT':
                    desc_list.remove(desc_b)
                elif desc_a['decl_type'] == 'COMPAT' and desc_b['decl_type'] == 'STD':
                    desc_list.remove(desc_a)
                else:
                    die("unrecognized duplication for" + desc_a['name'])
def parse_spec():
    line_number = 0
    partial_line = None
    spec_line = None
    handler_description_list = []

    for line in sys.stdin:
        if verbose:
            sys.stdout.write("\n" + str(line_number) + " >> " +  line)
        line_number += 1
        line = line.strip()

        if not partial_line:
            if line == '':
                parse_newline(line)
            elif line.startswith("#"):
                parse_preprocessor(line)
            elif line.startswith(";") or line.startswith("$"):
                parse_spec_comment(line)
            elif line[0].isdigit():
                if line.endswith("\\"):
                    partial_line = line
                    debug("partial line:", line)
                elif '{' in line and '}' in line:
                    spec_line = line
                else:
                    parse_unused(line)
            else:
                die("parse error")
        else:
            if line.endswith("\\"):
                partial_line = partial_line + line
                debug("partial line:", partial_line)
            else:
                partial_line = partial_line + line
                debug("partial line:", partial_line)
                spec_line = partial_line.replace("\\","")
                partial_line = None

        if spec_line:
            finished = spec_line
            spec_line = None
            try:
                handler_desc = parse_spec_line(finished)
                if handler_desc:
                    handler_description_list.append(handler_desc)
            except:
                debug("Unknown parsing error on line %d" % line_number)
                raise

    remove_duplicates(handler_description_list)
    return handler_description_list

def generate_preambles():
    preamble = """
/*** !!! this code has been autogenerated by utils/gen_sal.py, do not modify it directly. !!! ***/

"""
    c_output(preamble)
    h_output(preamble)

def generate_header_file(generated):
    index = 0x000A000
    h_output("#ifndef __EVENTS_GEN_H")
    h_output("#define __EVENTS_GEN_H\n\n")
    h_output("#define RREVENT_TABLE_GEN\\")
    last = generated[-1]
    for name in generated[:-1]:
        h_output("\tRREVENT(%s,\t0x%X)\\" % (name.upper(), index))
        index += 1
    h_output("\tRREVENT(%s,\t0x%X)" % (last.upper(), index))

    h_output("#endif")

def generate_includes():
    with open('autogenerate_includes') as f:
            includes_list = f.read().splitlines()
    for include in includes_list:
        c_output(include)

def generate_bindings(generated):
    c_output("")
    for name in generated:
        c_output("BIND_REF(%s);" % name)

def parse_flags():
    global verbose
    if len(sys.argv) == 2 and sys.argv[1] == '-v':
        verbose = True

def read_autogenerate_list():
    with open('autogenerate_syscalls') as f:
                autogenerate_list = f.read().splitlines()
    autogenerate_list = map(lambda x: x.strip(), autogenerate_list)
    autogenerate_list = filter(lambda x:not x.startswith(';') and len(x) > 0, autogenerate_list)
    return autogenerate_list

def format_handlers():
    subprocess.call(["indent","-i4", HANDLER_PATH])
    backup_path = HANDLER_PATH + ".BAK"
    os.unlink(backup_path)
    print "...formatting complete..."

if __name__ == '__main__':
    generated = []
    parse_flags()
    generate_preambles()
    generate_includes()
    autogenerate_list = read_autogenerate_list()
    print "Generating event handlers: " + str(autogenerate_list)
    handler_description_list = parse_spec()
    debug("desc:", handler_description_list)
    for desc in handler_description_list:
        if desc['name'] in autogenerate_list:
            generate_handler(desc)
            generated = generated + [desc['name']]
    generate_bindings(generated)
    generate_header_file(generated)
    missing = set(autogenerate_list) - set(generated)
    if len(missing) != 0:
        sys.exit("Failed to generate: %s " % str(list(missing)))
    cout.close()
    hout.close()
    format_handlers()
