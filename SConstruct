
opts = Variables('Local.sc')
opts.AddVariables(
    EnumVariable("BUILDTYPE", "Build Type", "DEBUG", ["DEBUG", "RELEASE"]),
    ("CC", "C Compiler"),
    ("CXX", "C++ Compiler"),
    ("AS", "Assembler"),
    ("LINK", "Linker"),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsx"])
)

objlib = Builder(action = 'ld -r -o $TARGET $SOURCES',
                 suffix = '.o',
                 src_suffix = '.c',
                 src_builder = 'StaticObject')

env = Environment(options = opts, BUILDERS = {'ObjectLibrary' : objlib})
env.Append(CXXFLAGS = ["-std=c++11"])
env.Append(CPPFLAGS = ["-g"])

if (env["BUILDTYPE"] == "DEBUG"):
    env.Append(CPPFLAGS = ["-DCASTOR_DEBUG"])
elif (env["BUILDTYPE"] == "RELEASE"):
    env.Append(CPPFLAGS = ["-DCASTOR_RELEASE"])
else:
    print "Unknown BUILDTYPE"

env.Append(CPPPATH = ["#include", "#include/" + env["RR"]])

Export('env')

SConscript("librr/SConstruct", variant_dir="build/librr")
SConscript("libsnap/SConstruct", variant_dir="build/libsnap")
SConscript("rrlog/SConstruct", variant_dir="build/rrlog")
SConscript("record/SConstruct", variant_dir="build/record")

AlwaysBuild(Alias('test', "build/librr/librr.o", "test/testbench.py"))

