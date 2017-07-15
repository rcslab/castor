
import os
import shutil
import SCons

def copy_file(target, source, env):
    dest = str(target[0])
    src = str(source[0])
    print dest
    print src
    shutil.copy(src, dest)

def generate(env, **kwargs):
    env['BUILDERS']['CopyFile'] = SCons.Builder.Builder(action = copy_file,
            source_factory=SCons.Node.FS.default_fs.Entry,
            target_factory=SCons.Node.FS.default_fs.Entry,
            multi=0)

def exists(env):
    return True

