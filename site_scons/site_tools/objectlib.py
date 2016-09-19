
import SCons

def generate(env, **kwargs):
    env['BUILDERS']['ObjectLibrary'] = SCons.Builder.Builder(
                action = SCons.Action.Action(
                            'ld -r -o $TARGET $SOURCES','$OBJLIBCOMSTR'),
                suffix = '.o',
                src_suffix = '.c',
                src_builder = 'StaticObject')

def exists(env):
    return True

