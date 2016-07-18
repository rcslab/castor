
opts = Variables('Local.sc')
opts.AddVariables(
    EnumVariable("BUILDTYPE", "Build Type", "DEBUG", ["DEBUG", "RELEASE"]),
    ("CC", "C Compiler"),
    ("CXX", "C++ Compiler"),
    ("AS", "Assembler"),
    ("LINK", "Linker"),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsc", "tsx"]),
    EnumVariable("CFG", "R/R Log Config", "ft", ["ft", "dbg", "snap"])
)

objlib = Builder(action = 'ld -r -o $TARGET $SOURCES',
                 suffix = '.o',
                 src_suffix = '.c',
                 src_builder = 'StaticObject')

env = Environment(options = opts, BUILDERS = {'ObjectLibrary' : objlib})
Help(opts.GenerateHelpText(env))

env.Append(CXXFLAGS = ["-std=c++11"])
env.Append(CPPFLAGS = ["-Wall", "-Werror", "-g", "-O2"])

if (env["BUILDTYPE"] == "DEBUG"):
    env.Append(CPPFLAGS = ["-DCASTOR_DEBUG"])
elif (env["BUILDTYPE"] == "RELEASE"):
    env.Append(CPPFLAGS = ["-DCASTOR_RELEASE"])
else:
    print "Unknown BUILDTYPE"

if (env["RR"] == "ctr"):
    env.Append(CPPFLAGS = ["-DCASTOR_CTR"])
elif (env["RR"] == "tsc"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSC"])
elif (env["RR"] == "tsx"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSX"])
else:
    print "Unknown RR"

if (env["CFG"] == "ft"):
    env.Append(CPPFLAGS = ["-DCASTOR_FT"])
elif (env["CFG"] == "dbg"):
    env.Append(CPPFLAGS = ["-DCASTOR_DBG"])
elif (env["CFG"] == "snap"):
    env.Append(CPPFLAGS = ["-DCASTOR_SNAP"])
else:
    print "Unknown CFG"

env.Append(CPPPATH = ["#include", "#include/" + env["RR"]])

Export('env')

SConscript("librr/SConstruct", variant_dir="build/librr")
SConscript("libsnap/SConstruct", variant_dir="build/libsnap")
SConscript("rrlog/SConstruct", variant_dir="build/rrlog")
SConscript("record/SConstruct", variant_dir="build/record")
SConscript("test/SConstruct", variant_dir="build/test")

AlwaysBuild(Alias('test', "build/librr/librr.o", "test/testbench.py"))

