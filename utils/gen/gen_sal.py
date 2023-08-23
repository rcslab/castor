#!/usr/bin/env python

from __future__ import print_function
import sys
import os
import re
import subprocess
import copy

#In order of precendence -- we will prefer the STD declaration over COMPAT, and so forth.
SUPPORTED_DECL_TYPES = ['STD', 'NOSTD']

RETURNS_BYTES_OUT = ['getdents', 'getdirentries', 'read', 'pread', 'readv', 'preadv','kenv']
RETURNS_ELEMENTS_OUT = ['kevent', 'getgroups']
ALWAYS_SUCCESSFUL_SYSCALLS = [ 'getegid', 'geteuid', 'getgid', 'getpgid', \
        'getpgrp', 'getpid', 'getppid', 'getuid', 'issetugid', 'umask']
CAST_SYSCALL_RETURN_TYPE = ['uid_t', 'gid_t']

INCLUDES_PATH = "./autogenerate_includes.h"
MISSING_CALLS_PATH = "./missing_syscalls.out"

TYPE_SIZES_C_PATH = "./type_sizes.c"
TYPE_SIZES_OUT_PATH = "./type_sizes.out"

HANDLER_PATH =  "./events_gen.c"
HEADER_PATH  =  "./events_gen.h"
PRINTER_CPATH =  "./events_pretty_printer_gen.c"
PRINTER_HPATH =  "./events_pretty_printer_gen.h"

cout = open(HANDLER_PATH, "w")
hout = open(HEADER_PATH, "w")
ppc_out = open(PRINTER_CPATH, "w")
pph_out = open(PRINTER_HPATH, "w")

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

def ppc_output(line):
    if debug_flag:
        debug("ppc_out", line)
    line = line + '\n'
    ppc_out.write(line)

def pph_output(line):
    if debug_flag:
        debug("pph_out", line)
    line = line + '\n'
    pph_out.write(line)

def scalar_arg_p(arg):
    result = arg['log_spec'] != None and arg['log_spec']['scalar'] == True
    debug("scalar?:" + str(result) + ":" + str(arg))
    return result

def gen_log_values(spec, type_size_map, mode):
    assert(mode in ['record','replay'])

    name = spec['name']
    return_type = spec['return_type']
    arg_names = [arg['name'] for arg in spec['args_spec']]
    arg_types =  [arg['type'] for arg in spec['args_spec']]

    #objectid
    leading_object = arg_types and (arg_types[0] == 'int')
    if leading_object and mode == 'record':
        c_output("e->objectId = (uint64_t)%s;" % arg_names[0])
    elif leading_object and mode == 'replay':
        c_output("AssertObject(e, (uint64_t)%s);" % arg_names[0])

    #result
    if mode == 'record':
        c_output("e->value[0] = (uint64_t)result;")
    else:
        c_output("result = (%s)e->value[0];" % return_type)
    next_value = 1

    #errno
    store_errno = False
    if not name in ALWAYS_SUCCESSFUL_SYSCALLS:
        store_errno = True
        c_output("if (result == -1) {")
        if mode == 'record':
            c_output("e->value[%d] = (uint64_t)errno;" % next_value)
        else:
            c_output("errno = e->value[%d];" % next_value)
        next_value += 1


    #store scalar params with side effects?
    SIZEOF_VALUE_FIELD = 8
    values_to_store = False
    for arg in spec['args_spec']:
        if scalar_arg_p(arg):
            base_type = arg['type'].split('*')[0].strip()
            if not weird_type_p(base_type) and type_size_map[base_type] <= SIZEOF_VALUE_FIELD:
                values_to_store = True
                break

    if values_to_store:
        c_output("} else {")
    elif store_errno:
        c_output("}") #close errno braces

    #scalar args
    for arg in spec['args_spec']:
        if scalar_arg_p(arg) and next_value < 5:
            base_type = arg['type'].split('*')[0].strip()
            if not weird_type_p(base_type) and type_size_map[base_type] <= SIZEOF_VALUE_FIELD:
                null_check = arg['log_spec']['null_check']
                if null_check:
                    c_output("if (%s != NULL) {" % arg['name'])
                if mode == 'record':
                    c_output("e->value[%d] = (uint64_t)(*%s);" % (next_value, arg['name']))
                else:
                    c_output("(*%s) = (%s)e->value[%d];" % (arg['name'], base_type, next_value))
                if null_check:
                    c_output("}")
                arg['log_spec']['logged'] = True
                next_value += 1

    if values_to_store:
        c_output("}") #close else braces

    return spec

def log_aggregate_p(arg):
    return arg['log_spec'] != None and not arg['log_spec']['logged']

def logged_data_p(spec):
    for arg in spec['args_spec']:
        if log_aggregate_p(arg):
            return True
    return False

def gen_log_data(spec):
    if logged_data_p(spec):
        c_output("if (result != -1) {")
        for arg in spec['args_spec']:
            if log_aggregate_p(arg):
                if arg['log_spec']['null_check']:
                    c_output("if (%s != NULL) {" % (arg['name']))
                c_output("logData((uint8_t *)%s, (unsigned long) %s);" %
                            (arg['name'], arg['log_spec']['size']))
                if arg['log_spec']['null_check']:
                    c_output("}")
        c_output("}")

def generate_handler(spec, type_size_map):
    debug("generating_handler(%s) with spec:%s", (spec['name'], str(spec)))
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
    c_output("e = RRLog_Alloc(rrlog, getThreadId());")
    c_output("e->event = %s;" % event_number)
    spec = gen_log_values(spec, type_size_map, 'record')
    c_output("RRLog_Append(rrlog, e);")
    gen_log_data(spec)
    c_output("break;")

    #replay mode
    c_output("case RRMODE_REPLAY:")
    c_output("e = RRPlay_Dequeue(rrlog, getThreadId());")
    c_output("AssertEvent(e, %s);" % event_number)
    spec = gen_log_values(spec, type_size_map, 'replay')
    c_output("RRPlay_Free(rrlog, e);")
    gen_log_data(spec)
    c_output("break;")

    c_output("}") #end switch
    c_output("return result;")
    c_output("}") #end function
    return spec

def generate_pretty_printer(spec):
    debug("generating_handler(%s) with spec:%s", (spec['name'], str(spec)))
    name = spec['name']
    return_type = spec['return_type']
    arg_types =  [arg['type'] for arg in spec['args_spec']]
    arg_names =  [arg['name'] for arg in spec['args_spec']]
    leading_object = arg_types and (arg_types[0] == 'int')

    return_format = {'int':'%d', 'ssize_t':'%zd', 'long':'%ld',
            'pid_t':'%d','uid_t':'%d','gid_t':'%d', 'mode_t':'%d',
            '__off_t':'%ld', 'off_t':'%ld'}[return_type]

    ppc_output("void")
    ppc_output("pretty_print_%s(RRLogEntry entry) {" % name.upper())

    arg_format = '()'
    arg_values = ''
    logged_names = []

    fmt_str_idx = 0
    arg_str_count = 0
    printf_arg = []
    printf_arg_err = []
    printf_arg_fmt = []
    printf_arg_fmt_err = []
    arg_expanded = False

    ppc_output('printf("' + name + '");')

    if logged_data_p(spec):
        ppc_output('if ((int)entry.value[0] != -1) {')
    
    for arg in spec['args_spec']:
        if leading_object:
            printf_arg_fmt.append('%d')
            printf_arg_fmt_err.append('%d')
            printf_arg.append('(int)entry.objectId') 
            leading_object = False
            continue
        
        arg_expanded = False
        if log_aggregate_p(arg) and not arg['log_spec']['null_check']:
            #XXX make this an exact comparison
            if 'struct stat' in arg['type']:
                str_name = 'str' + str(arg_str_count)

                ppc_output('struct stat %s;' % arg['name'])
                ppc_output('char ' + str_name + '[255];')
                ppc_output('readData((uint8_t *)&%s,sizeof(%s));' % (arg['name'], arg['name']))
                ppc_output('castor_xlat_stat(%s,%s);\n' % (arg['name'], str_name))

                printf_arg_fmt.append('%s')
                printf_arg.append(str_name)
                arg_str_count += 1
                arg_expanded = True

        if arg['log_spec']:
            printf_arg_fmt_err.append(arg['name'])
        else:
            printf_arg_fmt_err.append('_')

        if not arg_expanded:
            if arg['log_spec']:
                printf_arg_fmt.append(arg['name'])
            else:
                printf_arg_fmt.append('_')

    for fmt in printf_arg_fmt_err: 
        if fmt == '%s' or fmt == '%d':
            printf_arg_err.append(printf_arg[fmt_str_idx])
            fmt_str_idx += 1

    fmt_str = ", ".join(printf_arg_fmt)
    fmt_str_err = ", ".join(printf_arg_fmt_err)
    print_str = ", ".join(printf_arg)
    print_str_err = ", ".join(printf_arg_err)

    if len(print_str) > 0:
        ppc_output('printf("(' + fmt_str + '", ' + print_str + ');')
    else:
        ppc_output('printf("(' + fmt_str + '");')

    if logged_data_p(spec):
        ppc_output('} else {')
        
        if len(print_str_err) > 0:
            ppc_output('printf("(' + fmt_str_err + '", ' + print_str_err + ');')
        else:
            ppc_output('printf("(' + fmt_str_err + '");')

        ppc_output('}')

    ppc_output('printf(") = ' + return_format + '", (' + return_type + ')entry.value[0]);\n')

    if not name in ALWAYS_SUCCESSFUL_SYSCALLS:
        ppc_output('if ((int)entry.value[0] == -1) {')
        ppc_output('printf(" [errno: %s]", castor_xlat_errno((int)entry.value[1]));')
        ppc_output("}\n")

    ppc_output('printf("\\n");')
    ppc_output('}\n')

def generate_builtin_printers(builtins):
    for name in builtins:
        ppc_output("void")
        ppc_output("pretty_print_%s(RRLogEntry entry) {" % name.upper())
        if name == 'data':
            ppc_output('printf("[builtin] <%s>\\n");' % name)
        else:
            ppc_output('printf("[builtin] %s()\\n");' % name)
        ppc_output("}\n")

def parse_logspec(sal, type, syscall_name):
    debug("parse_logspec(%s, %s)" % (str(sal), type))
    sal_tag = sal['tag']
    sal_arg = sal['arg']

    if sal_tag.startswith('_In_'):
       return None

    log_spec = {'sal_tag' : sal_tag, 'scalar': False, 'logged': False}
    log_spec['null_check'] = '_opt_' in sal_tag
    sal_tag = re.sub("opt_","",sal_tag)

    if sal_tag in ['_Out_', '_Inout_']:
        size_type = type.split('*')[0].strip()
        debug("appending: " + size_type)
        log_spec['size'] = "sizeof(%s)" % size_type
        log_spec['scalar'] = True
    elif sal_tag in ['_Out_writes_bytes_', '_Out_writes_z_', \
                        '_Inout_updates_bytes_', '_Inout_updates_z_']:
        bytes_out = 'result' if syscall_name in RETURNS_BYTES_OUT else sal_arg
        log_spec['size'] = bytes_out
    elif sal_tag in ['_Out_writes_', '_Inout_updates_']:
        elements_out = 'result' if syscall_name in RETURNS_ELEMENTS_OUT else sal_arg
        base_type = type.split('*')[0].strip()
        log_spec['size'] = "%s * sizeof(%s)" % (elements_out, base_type)
    else:
        error("Unknown SAL annotation:" + str(sal))

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
        type = arg.strip()
        arg_spec['type'] = type
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

def pre_parse_spec_line(line):
    preparsed_line = line

    # remove extra commas before )
    pivot = preparsed_line.rfind(')')
    last_comma = line.rfind(',', 0, pivot)
    if pivot != -1 and last_comma != -1:
        content = preparsed_line[last_comma: pivot]
        match = re.search(r'[a-zA-Z]+', content) or re.search(r'[0-9]+', content)
        if not match:
            preparsed_line = preparsed_line[:last_comma] + preparsed_line[last_comma+1:]

    return preparsed_line

def parse_spec_line(line):
    debug("spec_line:" + line)
    handler_spec = {}
    line = pre_parse_spec_line(line)
    pivot = line.index('{')
    prefix = line[:pivot]
    proto = line[pivot:]
    debug("prefix:" + prefix)
    debug("proto:" + proto)
    number, name, type = prefix.split()

    if "STD" in type and "CAPENABLED" in type:
        type = "STD"

    handler_spec['decl_type'] = type
    if not type in SUPPORTED_DECL_TYPES:
        debug("Unused line:" + line)
        return None
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
    after_brkt_line = False
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
                partial_line = line
                if '}' in line:
                    if not line.endswith("\\"):
                        spec_line = partial_line
                    else:
                        after_brkt_line = True
                    
                if (not '{' in line) and (not '}' in line):
                    partial_line = ""
                    parse_unused(line)
            else:
                die("Parse error on line:" + str(line_number))
        else:
            if '}' in line or after_brkt_line:
                partial_line = partial_line + line
                if not line.endswith("\\"):
                    debug("partial line:", partial_line)
                    after_brkt_line = False
                    spec_line = partial_line.replace("\\","")
                    partial_line = None
                else:
                    after_brkt_line = True
            else:
                partial_line = partial_line + line
                debug("partial line:", partial_line)

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
            partial_line = ""

    return syscall_description_list

def generate_preambles():
    preamble = """
/*** !!! this code has been autogenerated by utils/gen_sal.py, do not modify it directly. !!! ***/

"""
    c_output(preamble)
    h_output(preamble)
    ppc_output(preamble)
    pph_output(preamble)

def padding(str):
    return (20 - len(str)) * ' '

def generate_event_table(table_name, event_list, index):
    h_output("#define RREVENT_TABLE_%s\\" % table_name)
    start = index
    last = event_list[-1]
    for name in event_list[:-1]:
        h_output("\tRREVENT(%s,%s%u)\\" % (name.upper(), padding(name), index))
        index += 1
    h_output("\tRREVENT(%s,%s%u)" % (last.upper(), padding(name), index))
    end = index
    h_output("\n#define %s_CONTAINS(x) ((%s < x) && (x < %s))" % (table_name, start, end))
    return index + 1

def generate_header_file(builtin_events, builtin_syscalls, generated):
    index = 0
    h_output("#ifndef __EVENTS_GEN_H")
    h_output("#define __EVENTS_GEN_H\n")

    h_output("\n#define RREVENT(_a, _b) RREVENT_##_a = _b,\n")

    index = generate_event_table("BUILTIN_EVENTS", builtin_events, index)
    h_output("\n")

    index = generate_event_table("BUILTIN_SYSCALLS", builtin_syscalls, index)
    h_output("\n")

    index = generate_event_table("GENERATED_SYSCALLS", generated, index)
    h_output("\n#define RREVENTS_MAX %s" % index)


    h_output("\n#define RREVENT_TABLE RREVENT_TABLE_%s RREVENT_TABLE_%s RREVENT_TABLE_%s" %
            ("BUILTIN_EVENTS", "BUILTIN_SYSCALLS", "GENERATED_SYSCALLS"))

    h_output("\nenum { RREVENT_TABLE };")
    h_output("\n#undef RREVENT")

    h_output("\n#endif")

def generate_pretty_print_header_file(builtin, generated):
    pph_output("#ifndef __CASTOR_PRETTY_PRINT__")
    pph_output("#define __CASTOR_PRETTY_PRINT__")

    for name in generated + builtin:
        pph_output("extern void pretty_print_%s(RRLogEntry entry);" % name.upper())

    pph_output("#endif")

def import_includes():
    with open(INCLUDES_PATH) as f:
            includes_list = f.read().splitlines()
    for include in includes_list:
        c_output(include)
        ppc_output(include)

def add_includes():
    ppc_output('#include "castor_xlat.h"')
    ppc_output('#include "rrlog.h"')

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

def weird_type_p(type):
    return type in ['void','...']

def read_syscall_list(file_name):
    syscall_list = []
    with open(file_name) as f:
                syscall_list = f.read().splitlines()
    syscall_list = map(lambda x: x.strip(), syscall_list)
    syscall_list = list(filter(lambda x:not x.startswith(';') and len(x) > 0, syscall_list))
    duplicates = find_duplicates(syscall_list)
    if len(duplicates) > 0:
        die(file_name + " contains duplicate entries:" + str(duplicates))
    return syscall_list

def format_handlers():
    subprocess.check_call(["indent","-i4", HANDLER_PATH])
    backup_path = HANDLER_PATH + ".BAK"
    os.unlink(backup_path)
    print("...formatted handlers...")

def format_pretty_printers():
    subprocess.check_call(["indent","-i4", PRINTER_CPATH])
    backup_path = PRINTER_CPATH + ".BAK"
    os.unlink(backup_path)
    print("...formated pretty printers...")

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

def read_syscall_type_signatures(specs):
    type_signatures = {}

    for spec in specs:
        name = spec['name']
        return_type = spec['return_type']
        args = []
        for spec_arg in spec['args_spec']:
            args.append(spec_arg['type'])
        if name in type_signatures:
            current = type_signatures[name]
            if (return_type != current['return_type'] or args != current['args']):
                die("conficting type signatures for :" + name )

        type_signatures[name] = {'name': name, 'return_type': return_type, 'args' : args}

    debug("type_signatures:", type_signatures)
    return type_signatures

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
                          "(?P<args>([\w\s\.[\],\*^)]+))\)", line)
        if match != None:
            return_type = match.group('return_type').strip()
            name = match.group('name').strip()
            args = re.sub('\n','',match.group('args')).split(',')
            args = list(map(lambda x:x.strip(), args))
            args = list(map(lambda x:clean_types(x), args))
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

def resolve_types(desc, type_signatures, type_list):
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
            type_list.append(sig_arg_type)

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

def build_type_size_map(type_list):
    type_size_map = {}
    base_types = map(lambda x: x.split('*')[0].strip(), type_list)
    base_types = list(set(base_types)) #remove duplicates
    base_types  = filter(lambda x: not weird_type_p(x), base_types)
    with open(TYPE_SIZES_C_PATH, "w" ) as f:
        f.write('#include \"autogenerate_includes.h\"\n')
        f.write('int main() {\n')
        for type in base_types:
            f.write('printf(\"%s,%%lu\\n\",sizeof(%s));\n' % (type,type))
        f.write('return 0; \n }')
    exec_path = TYPE_SIZES_C_PATH[:-2]
    subprocess.check_call(["cc", "-DGEN_SAL", TYPE_SIZES_C_PATH, "-o", exec_path])
    os.system("%s > %s" % (exec_path, TYPE_SIZES_OUT_PATH))
    with open(TYPE_SIZES_OUT_PATH, "r" ) as f:
        for line in f:
            type, size = line.split(',')
            size = size.strip()
            type_size_map[type] = int(size)
    if not debug_flag:
        os.unlink(TYPE_SIZES_C_PATH)
        os.unlink(TYPE_SIZES_OUT_PATH)
        os.unlink(exec_path)

    return type_size_map

if __name__ == '__main__':
    generated = []
    parse_flags()
    specs = parse_spec()

    type_signatures_from_libc = read_libc_type_signatures()
    type_signatures_from_syscall = read_syscall_type_signatures(specs)
    type_signatures = {}

    for sig in type_signatures_from_syscall:
        if sig in type_signatures_from_libc:
            type_signatures[sig] = type_signatures_from_libc[sig]
        else:
            type_signatures[sig] = type_signatures_from_syscall[sig]

    syscall_description_list = remove_duplicate_descriptions(specs)

    all_syscalls_list = map(lambda x:x['name'], syscall_description_list)

    syscalls_list = read_syscall_list('autogenerate_syscalls')
    passthrough_list = read_syscall_list('passthrough_syscalls')
    unimplemented_list = read_syscall_list('unimplemented_syscalls')
    unsupported_list = read_syscall_list('unsupported_syscalls')
    builtin_syscalls = read_syscall_list('builtin_syscalls')
    builtin_events = read_syscall_list('builtin_events')

    autogenerate_list = list(filter(lambda i: i not in unimplemented_list, syscalls_list))
    autogenerate_list = list(filter(lambda i: i not in unsupported_list, autogenerate_list))

    builtin_syscalls = list(filter(lambda i: i not in unimplemented_list, builtin_syscalls))
    builtin_syscalls = list(filter(lambda i: i not in unsupported_list, builtin_syscalls))

    builtin_events = list(filter(lambda i: i not in unimplemented_list, builtin_events))
    builtin_events = list(filter(lambda i: i not in unsupported_list, builtin_events))

    core_runtime_list = subprocess.check_output('./core_runtime_syscalls.sh').split()
    duplicates = find_duplicates(autogenerate_list + passthrough_list + \
            unimplemented_list + unsupported_list + core_runtime_list)
    if len(duplicates) > 0:
        warning("syscall lists contain duplicates:" + str(duplicates))

    missing_list = list(set(all_syscalls_list) - set(autogenerate_list) - \
                        set(passthrough_list) - set(unsupported_list) - \
                        set(core_runtime_list) - set(unimplemented_list))

    generate_preambles()
    import_includes()
    add_includes()
    print("Generating syscall handlers...")
    debug("syscall_description_list:", syscall_description_list)
    processed_descriptions = []
    type_list = []

    #process builtin_syscalls and declared syscalls
    for desc in syscall_description_list:
        if desc['name'] in autogenerate_list + builtin_syscalls:
            desc = resolve_types(desc, type_signatures, type_list)
            desc = add_logspec(desc)
            processed_descriptions.append(desc)
    debug("type_list:", type_list)
    type_size_map = build_type_size_map(type_list)
    debug("type_size_map:", type_size_map)

    for desc in processed_descriptions:
        generate_pretty_printer(desc)
        if desc['name'] in autogenerate_list:
            desc = generate_handler(desc, type_size_map)
            generated = generated + [desc['name']]

    generate_builtin_printers(builtin_events)
    generate_bindings(generated)
    generate_header_file(builtin_events, builtin_syscalls, generated)
    generate_pretty_print_header_file(builtin_events + builtin_syscalls, generated)

    cout.close()
    hout.close()
    ppc_out.close()
    pph_out.close()

    missing = set(autogenerate_list) - set(generated)
    if len(missing) != 0:
        sys.exit("Failed to generate: %s " % str(list(missing)))

    if (error_count > 0):
        sys.exit("\n... Generation failed due to errors...\n")

    format_handlers()
    format_pretty_printers()

    print("==Summary==")
    print("System calls of type %s : (%s)" % \
        (str(SUPPORTED_DECL_TYPES), len(list(all_syscalls_list))))
    print("Core Runtime calls:(%s)" % len(core_runtime_list))
    print("Autogenerated calls:(%s)" % len(autogenerate_list))
    print("Passthrough calls:(%s)" % len(passthrough_list))
    print("Unimplemented calls:(%s)" % len(unimplemented_list))
    print("Unsupported calls:(%s)" % len(unsupported_list))
    print("Missing calls:(%s)" % len(missing_list))

    if len(missing_list) > 0:
        output_missing_calls(missing_list)

