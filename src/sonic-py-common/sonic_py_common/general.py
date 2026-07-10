import sys
from subprocess import Popen, STDOUT, PIPE, CalledProcessError, check_output


def load_module_from_source(module_name, file_path):
    """
    This function will load the Python source file specified by <file_path>
    as a module named <module_name> and return an instance of the module
    """
    module = None

    # TODO: Remove this check once we no longer support Python 2
    if sys.version_info.major == 3:
        import importlib.machinery
        import importlib.util
        loader = importlib.machinery.SourceFileLoader(module_name, file_path)
        spec = importlib.util.spec_from_loader(loader.name, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
    else:
        import imp
        module = imp.load_source(module_name, file_path)

    sys.modules[module_name] = module

    return module


def getstatusoutput_noshell(cmd):
    """
    This function implements getstatusoutput API from subprocess module
    but using shell=False to prevent shell injection.
    Ref: https://github.com/python/cpython/blob/3.10/Lib/subprocess.py#L602
    """
    try:
        output = check_output(cmd, universal_newlines=True, stderr=STDOUT)
        exitcode = 0
    except CalledProcessError as ex:
        output = ex.output
        exitcode = ex.returncode
    if output[-1:] == '\n':
        output = output[:-1]
    return exitcode, output


def getstatusoutput_noshell_pipe(cmd0, *args):
    """
    This function implements getstatusoutput API from subprocess module
    but using shell=False to prevent shell injection. Input command
    includes two or more commands connected by shell pipe(s).
    """
    popens = [Popen(cmd0, stdout=PIPE, universal_newlines=True)]
    i = 0
    while i < len(args):
        popens.append(Popen(args[i], stdin=popens[i].stdout, stdout=PIPE, universal_newlines=True))
        popens[i].stdout.close()
        i += 1
    output = popens[-1].communicate()[0]
    if output[-1:] == '\n':
        output = output[:-1]

    exitcodes = [0] * len(popens)
    while popens:
        last = popens.pop(-1)
        exitcodes[len(popens)] = last.wait()

    return (exitcodes, output)


def check_output_pipe(cmd0, *args):
    """
    This function implements check_output API from subprocess module.
    Input command includes two or more commands connected by shell pipe(s)
    """
    popens = [Popen(cmd0, stdout=PIPE, universal_newlines=True)]
    i = 0
    while i < len(args):
        popens.append(Popen(args[i], stdin=popens[i].stdout, stdout=PIPE, universal_newlines=True))
        popens[i].stdout.close()
        i += 1
    output = popens[-1].communicate()[0]

    i = 0
    args_list = [cmd0] + list(args)
    while popens:
        current = popens.pop(0)
        exitcode = current.wait()
        if exitcode != 0:
            raise CalledProcessError(returncode=exitcode, cmd=args_list[i], output=current.stdout)
        i += 1

    return output

