
import os
import json
import SCons

def generate(env, **kwargs):
    def ClangCheckAll(target, source, env):
        for fl in source:
            try:
                f = fl.get_contents()
                j = json.loads(f)
                for x in j:
                    fn = x["file"]
                    print env.subst(env['CLANGCHECK_COMSTR'], source = fn)
                    result = os.system(env["CLANGTIDY"]+" "+fn)
            except IOError as e:
                return

    tidypath = env.WhereIs('clang-tidy39') or env.WhereIs('clang-tidy')
    env['CLANGTIDY'] = kwargs.get('CLANGTIDY', tidypath)
    env['CLANGCHECK_COMSTR'] = kwargs.get('CLANGCHECK_COMSTR',
                                          'Checking $SOURCE')

    bld = SCons.Builder.Builder(
                action = ClangCheckAll,
                src_suffix = '.csv')
    env['BUILDERS']['ClangCheckAll'] = bld

def exists(env):
    return True

