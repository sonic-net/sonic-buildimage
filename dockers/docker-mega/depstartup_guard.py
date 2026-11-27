#!/usr/bin/env python3
"""depstartup pending-start guard (plan section 12, auto-generated).

Wraps supervisord_dependent_startup to prevent the process respawn storm in
the mega container.  Before any ``supervisor.startProcess`` XML-RPC call
reaches the supervisor daemon, this wrapper checks the target process's
current state and silently skips the call if the process is already in
STARTING, RUNNING, or BACKOFF — the three transitional states in which a
redundant ``startProcess`` would either fail or be a no-op that confuses
the dependent-startup state machine.
"""
import sys
import xmlrpc.client

_orig_Method_call = xmlrpc.client._Method.__call__


def _guarded_Method_call(self, *args):
    # _Method stores the dotted RPC method name in the name-mangled
    # ``__name`` attribute (e.g. "supervisor.startProcess").
    # ``__send`` is the bound ``ServerProxy.__request`` method that
    # actually makes the HTTP call — we reuse it for getProcessInfo
    # so the guard uses the same connection/auth as the original call.
    try:
        method_name = self._Method__name
    except AttributeError:
        return _orig_Method_call(self, *args)

    if method_name == "supervisor.startProcess" and args:
        process_name = args[0]
        try:
            send = self._Method__send
            info = send("supervisor.getProcessInfo", (process_name,))
            state = info.get("statename", "")
            if state in ("STARTING", "RUNNING", "BACKOFF"):
                return True  # skip — already transitioning
        except Exception:
            pass  # if the check itself fails, let the start proceed

    return _orig_Method_call(self, *args)


xmlrpc.client._Method.__call__ = _guarded_Method_call

# Run the original supervisord_dependent_startup module as __main__.
import runpy
runpy.run_module("supervisord_dependent_startup", run_name="__main__",
                 alter_sys=True)
