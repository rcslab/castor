#!/usr/bin/env python

from __future__ import print_function
import sys
import os
import re
import subprocess
import copy

HANDLER_PATH = "./events_gen.c"
HEADER_PATH = "./events_gen.h"
INCLUDES_PATH = "./autogenerate_includes.h"
MISSING_CALLS_PATH = "./missing_syscalls.out"

#In order of precendence -- we will prefer the STD declaration over COMPAT, and so forth.
SUPPORTED_DECL_TYPES = ['STD', 'NOSTD', 'COMPAT', 'COMPAT11']

RETURNS_BYTES_OUT = ['getdents', 'getdirentries', 'read', 'pread', 'readv', 'preadv','kenv']
RETURNS_ELEMENTS_OUT = ['kevent', 'getgroups']
ALWAYS_SUCCESSFUL_SYSCALLS = [ 'getegid', 'geteuid', 'getgid', 'getpgid', \
        'getpgrp', 'getpid', 'getppid', 'getuid', 'issetugid', 'umask']
CAST_SYSCALL_RETURN_TYPE = ['uid_t', 'gid_t']

cout = open(HANDLER_PATH, "w")
hout = open(HEADER_PATH, "w")

debug_flag = False

error_count = 0
warning_count = 0

type_signatures = None

def die(message):
    raise ValueError(message)

def debug(*args, **kwargs):
    if debug_flag:
	print("DEBUG >>", *args, file=sys.stdout, **kwargs)

def error(*args, **kwargs):
    global error_count
    error_count += 1
    print("ERROR >>", *args, file=sys.stderr, **kwargs)

def warning(*args, **kwargs):
    global warning_count
    warning_count += 1
    print("WARNING >>", *args, file=sys.stderr, **kwargs)


def c_output(line):
    if debug_flag:
        debug("cout", line)
    line = line + '\n'
    cout.write(line)

def h_output(line):
    if debug_flag:
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
                c_output("\t\t\tlogData((uint8_t *)%s, (unsigned long) %s);" %
                            (arg['name'], arg['log_spec']['size']))
                if arg['log_spec']['null_check']:
                    c_output("\t\t\t }")
        c_output("\t\t}")

def generate_handler(spec):
    debug("handler_desc:", spec)
    name = spec['name']
    return_type = spec['return_type']
    arg_list = [arg['type'] + ' ' + arg['name'] for arg in spec['args_spec']]
    arg_names = [arg['name'] for arg in spec['args_spec']]
    arg_types =  [arg['type'] for arg in spec['args_spec']]
    arg_string = ", ".join(arg_list)
    syscall_number = "SYS_" + name
    event_number = "RREVENT_%s" % name.upper()
    leading_object = arg_types and (arg_types[0] == 'int')
    call_args = [syscall_number] + arg_names
    syscall_str =  "__rr_syscall(%s)" % ", ".join(call_args)

    if return_type in CAST_SYSCALL_RETURN_TYPE:
        syscall_str = "(%s) %s" % (return_type, syscall_str)

    #handler opening
    c_output("\n%s" % return_type)
    c_output("__rr_%s(%s)" % (name, arg_string))
    c_output("{")
    c_output("%s result;" % return_type)
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
    if not name in ALWAYS_SUCCESSFUL_SYSCALLS:
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
    c_output("result = (%s)e->value[0];" % return_type)
    if not name in ALWAYS_SUCCESSFUL_SYSCALLS:
        c_output("if (result == -1) {")
        c_output("errno = e->value[1];")
        c_output("}")

    c_output("RRPlay_Free(rrlog, e);")
    gen_log_data(spec)
    c_output("\t    break;")

    c_output("    }") #end switch

    c_output("return result;")
    c_output("}")


def parse_logspec(sal, type, syscall_name):
    debug("parse_logspec(%s, %s)" % (str(sal), type))
    sal_tag = sal['tag']
    sal_arg = sal['arg']

    log_spec = None
    null_check = '_opt_' in sal_tag
    sal_tag = re.sub("opt_","",sal_tag)

    if sal_tag.startswith('_In_'):
       pass
    elif sal_tag in ['_Out_', '_Inout_']:
        size_type = type.split('*')[0].strip()
        log_spec = { 'size' : "sizeof(%s)" % size_type }
    elif sal_tag in ['_Out_writes_bytes_', '_Out_writes_z_', \
                     '_Inout_updates_bytes_', '_Inout_updates_z_']:
        bytes_out = 'result' if syscall_name in RETURNS_BYTES_OUT else sal_arg
        log_spec = { 'size': bytes_out}
    elif sal_tag in ['_Out_writes_', '_Inout_updates_']:
        elements_out = 'result' if syscall_name in RETURNS_ELEMENTS_OUT else sal_arg
        base_type = type.split('*')[0].strip()
        log_spec = { 'size': "%s * sizeof(%s)" % (elements_out, base_type) }
    else:
        error("Unknown SAL annotation:" + str(sal))

    if log_spec != None:
        log_spec['null_check'] = null_check

    return log_spec


def parse_args(args_string, handler_spec):
    args_spec = []
    if args_string == 'void':
        return args_spec

    args = args_string.split(',')
    debug("args:", args)
    for i in range(0, len(args)):
        arg_spec = {}
        arg = args[i]
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
            debug("arg_sal:", str(sal))
        elif '*' in arg or 'caddr_t' in arg:
            error("Pointer argument %s with missing sal annotation in %s." % \
                    (name, handler_spec['name']))
        arg_spec['sal'] = sal

        #type is whats left
        arg_spec['type'] = arg.strip()
        debug("arg_type:" + arg_spec['type'])
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
    return_type, name = prefix.split()
    debug("name:" + name)
    debug("return_type:" + return_type)
    handler_spec['name'] = name
    handler_spec['return_type'] = return_type
    arg_string = suffix.strip('(); ')
    handler_spec['args_spec'] = parse_args(arg_string, handler_spec)
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
    if not type in SUPPORTED_DECL_TYPES:
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

def remove_duplicate_descriptions(desc_list):
    result_list = []
    seen = set([])
    syscall_names = map(lambda x:x['name'], desc_list)
    duplicates = find_duplicates(syscall_names)
    debug("duplicate descriptions for:" + str(duplicates))

    for desc in desc_list:
        if not desc['name'] in duplicates:
            result_list.append(desc)
            debug("Adding " + desc['name'])

    for name in set(duplicates):
        match = None
        for decl_type in SUPPORTED_DECL_TYPES:
            for desc in desc_list:
                if desc['decl_type'] == decl_type and desc['name'] == name:
                        match = desc
                        break
            if match != None:
                break
        if match:
            result_list.append(match)
            debug("Resolved duplicate for:", desc['name'])
        else:
            die("failed to remove duplicate for:", desc['name'])

    return result_list

def parse_spec():
    line_number = 0
    partial_line = None
    spec_line = None
    syscall_description_list = []

    for line in sys.stdin:
        if debug_flag:
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
                die("Parse error on line:" + str(line_number))
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
                syscall_desc = parse_spec_line(finished)
                if syscall_desc:
                    syscall_description_list.append(syscall_desc)
            except:
                debug("Unknown parsing error on line %d" % line_number)
                raise

    return syscall_description_list

def generate_preambles():
    preamble = """
/*** !!! this code has been autogenerated by utils/gen_sal.py, do not modify it directly. !!! ***/

"""
    c_output(preamble)
    h_output(preamble)

def padding(str):
    return (20 - len(str)) * ' '

def generate_header_file(generated):
    index = 0x10000000
    h_output("#ifndef __EVENTS_GEN_H")
    h_output("#define __EVENTS_GEN_H\n\n")
    h_output("#define RREVENT_TABLE_GEN\\")
    last = generated[-1]
    for name in generated[:-1]:
        h_output("\tRREVENT(%s,%s0x%X)\\" % (name.upper(), padding(name), index))
        index += 1
    h_output("\tRREVENT(%s,%s0x%X)" % (last.upper(), padding(name), index))

    h_output("#endif")

def generate_includes():
    with open(INCLUDES_PATH) as f:
            includes_list = f.read().splitlines()
    for include in includes_list:
        c_output(include)

def generate_bindings(generated):
    c_output("")
    for name in generated:
        c_output("BIND_REF(%s);" % name)

def parse_flags():
    global debug_flag
    if len(sys.argv) == 2 and sys.argv[1] == '-d':
        debug_flag = True

def find_duplicates(list):
    seen = set()
    duplicates = set()
    for i in list:
        if i in seen:
            duplicates.add(i)
        else:
            seen.add(i)
    return duplicates


def read_syscall_list(file_name):
    syscall_list = []
    with open(file_name) as f:
                syscall_list = f.read().splitlines()
    syscall_list = map(lambda x: x.strip(), syscall_list)
    syscall_list = filter(lambda x:not x.startswith(';') and len(x) > 0, syscall_list)
    duplicates = find_duplicates(syscall_list)
    if len(duplicates) > 0:
        die(file_name + " contains duplicate entries:" + str(duplicates))
    return syscall_list

def format_handlers():
    subprocess.check_call(["indent","-i4", HANDLER_PATH])
    backup_path = HANDLER_PATH + ".BAK"
    os.unlink(backup_path)
    print("...formatting complete...")

def clean_types(arg):
    #get replace array signatures with pointers
    arg = re.sub('\w*\[\]','*',arg)

    #XXX half ass heuristic to remove variable names
    parts = arg.split()
    result = parts
    if len(parts) == 1:
        pass
    elif '*' in arg:
        result = arg.split('*')[:-1] + ['*']
    elif len(parts) > 1:
        if parts[-2] in ['char', 'short', 'int', 'long'] \
                or parts[-2].endswith('_t'):
                    result = parts[:-1]
    return ' '.join(result)

def read_libc_type_signatures():
    type_signatures = {}
    output_path = INCLUDES_PATH[:-1] + "E"
    subprocess.check_call(["cc","-E","-P", "-DGEN_SAL", INCLUDES_PATH, "-o", output_path])
    src_lines = []
    with open(output_path) as f:
        line = f.readline()
        while line != '':
            while re.search(',$',line) != None:
                line = line.strip() + f.readline().lstrip()
            debug("src_line:" + line)
            src_lines.append(line)
            line = f.readline()

    for line in src_lines:
        match = re.search("(?P<return_type>(^[\w\s\*]+))\s"\
                          "(?P<name>(\w+))\("\
                          "(?P<args>([\w\s\[\],\*^)]+))\)", line)
        if match != None:
            return_type = match.group('return_type').strip()
            name = match.group('name').strip()
            args = re.sub('\n','',match.group('args')).split(',')
            args = map(lambda x:x.strip(), args)
            args = map(lambda x:clean_types(x), args)
            if name in type_signatures:
                current = type_signatures[name]
                if (return_type != current['return_type'] or args != current['args']):
                    die("conficting type signatures for :" + name )
            type_signatures[name] = {'name': name, 'return_type': return_type, 'args' : args}
        else:
            debug("no match:" + line)
    if not debug_flag:
        os.unlink(output_path)
    debug("type_signatures:", type_signatures)
    return type_signatures

def add_logspec(desc):
    logged_desc = copy.deepcopy(desc)
    for arg_spec in logged_desc['args_spec']:
        if arg_spec['sal'] != None:
            arg_spec['log_spec'] = parse_logspec(arg_spec['sal'], arg_spec['type'], desc['name'])
        else:
            arg_spec['log_spec'] = None
    return logged_desc

def resolve_types(desc, type_signatures):
    resolved_desc = copy.deepcopy(desc)
    name = desc['name']
    libc_name = re.sub("^__", "", name)
    if libc_name in type_signatures:
        sig = type_signatures[libc_name]
    else:
        die("Missing type signature for: \'%s\', you might be"\
                "missing an entry in autogenerate_includes.h" %  desc['name'])
    args_spec = desc['args_spec']

    if args_spec == [] and sig['args'] == ['void']:
        pass
    elif len(args_spec) != len(sig['args']):
        print("number of args mismatch for " + name)
        print("args_spec:", args_spec)
        print("sig['args']", sig['args'])
        die("can't resolve types")
    else:
        for i in range(0, len(args_spec)):
            sig_arg_type = sig['args'][i]
            spec_arg_type = args_spec[i]['type']
            if sig_arg_type != spec_arg_type:
                debug("type_mismatch in %s, in arg %s, %s != %s: resolving to %s" %
                        (name, i, spec_arg_type, sig_arg_type, sig_arg_type))
                resolved_desc['args_spec'][i]['type'] = sig_arg_type

    if sig['return_type'] != desc['return_type']:
        debug("type_mismatch in %s, return type %s != %s: resolving to %s" %
                (name, desc['return_type'], sig['return_type'], sig['return_type']))
        resolved_desc['return_type'] = sig['return_type']
    return resolved_desc

def output_missing_calls(missing_list):
    opening = "; this list has been autogenerated,do not modify it manually!!!\n"\
              "; to refresh re-run the autogenerator\n"
    missing_list.sort()
    with open(MISSING_CALLS_PATH, "w" ) as f:
        f.write(opening)
        f.write('\n'.join(missing_list))
    print("\n...Updated missing calls list written to \'%s\' ..." % MISSING_CALLS_PATH)

if __name__ == '__main__':
    generated = []
    parse_flags()

    type_signatures = read_libc_type_signatures()
    syscall_description_list = remove_duplicate_descriptions(parse_spec())

    all_syscalls_list = map(lambda x:x['name'], syscall_description_list)

    autogenerate_list = read_syscall_list('autogenerate_syscalls')
    passthrough_list = read_syscall_list('passthrough_syscalls')
    unimplemented_list = read_syscall_list('unimplemented_syscalls')
    unsupported_list = read_syscall_list('unsupported_syscalls')
    core_runtime_list = subprocess.check_output('./core_runtime_syscalls.sh').split()
    duplicates = find_duplicates(autogenerate_list + passthrough_list +\
            unimplemented_list + unsupported_list + core_runtime_list)
    if len(duplicates) > 0:
        warning("syscall lists contain duplicates:" + str(duplicates))

    missing_list = list(set(all_syscalls_list) - set(autogenerate_list) - \
                        set(passthrough_list) - set(unsupported_list) - \
                        set(core_runtime_list) - set(unimplemented_list))

    generate_preambles()
    generate_includes()
    print("Generating syscall handlers...")
    debug("syscall_description_list:", syscall_description_list)
    for desc in syscall_description_list:
        if desc['name'] in autogenerate_list:
            desc = resolve_types(desc, type_signatures)
            desc = add_logspec(desc)
            generate_handler(desc)
            generated = generated + [desc['name']]
    generate_bindings(generated)
    generate_header_file(generated)
    cout.close()
    hout.close()

    missing = set(autogenerate_list) - set(generated)
    if len(missing) != 0:
        sys.exit("Failed to generate: %s " % str(list(missing)))

    if (error_count > 0):
        sys.exit("\n... Generation failed due to errors...\n")

    format_handlers()

    print("==Summary==")
    print("System calls of type %s : (%s)" % \
        (str(SUPPORTED_DECL_TYPES), len(all_syscalls_list)))
    print("Core Runtime calls:(%s)" % len(core_runtime_list))
    print("Autogenerated calls:(%s)" % len(autogenerate_list))
    print("Passthrough calls:(%s)" % len(passthrough_list))
    print("Unimplemented calls:(%s)" % len(unimplemented_list))
    print("Unsupported calls:(%s)" % len(unsupported_list))
    print("Missing calls:(%s)" % len(missing_list))

    if len(missing_list) > 0:
        output_missing_calls(missing_list)

