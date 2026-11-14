"""Bounded, fail-fast VTY transport isolated from the shared BGP client."""

from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
import socket
import time


DAEMONS = frozenset(('isisd', 'zebra', 'mgmtd', 'staticd'))


class NativeError(RuntimeError):
    pass


@dataclass(frozen=True)
class Operation:
    daemon: str
    commands: tuple


class NativeClient:
    """Use fresh scoped sessions; a lost reply never leaves a reusable context.

    Commands are sent individually and their complete status is checked before
    the next command. No BGP socket or shared command-manager lock is involved.
    """

    def __init__(self, directory='/run/frr', timeout=2.0, max_reply=4 * 1024 * 1024,
                 socket_factory=socket.socket, clock=time.monotonic):
        self.directory = Path(directory)
        self.timeout, self.max_reply = float(timeout), int(max_reply)
        self.socket_factory, self.clock = socket_factory, clock
        if self.timeout <= 0 or self.max_reply < 4:
            raise ValueError('Invalid native transport bounds')

    def _command(self, connection, command, deadline):
        if not isinstance(command, str) or not command or '\x00' in command or '\n' in command:
            raise NativeError('Invalid VTY command')
        remaining = deadline - self.clock()
        if remaining <= 0:
            raise NativeError('Native operation deadline expired')
        connection.settimeout(min(self.timeout, remaining))
        connection.sendall(command.encode('utf-8') + b'\0')
        reply = bytearray()
        while True:
            remaining = deadline - self.clock()
            if remaining <= 0:
                raise NativeError('Native reply deadline expired')
            connection.settimeout(min(self.timeout, remaining))
            chunk = connection.recv(min(16384, self.max_reply + 1 - len(reply)))
            if not chunk:
                raise NativeError('Native connection closed before complete reply')
            reply.extend(chunk)
            if len(reply) > self.max_reply:
                raise NativeError('Native reply exceeds limit')
            if len(reply) >= 4 and reply[-4:-1] == b'\0\0\0':
                code = reply[-1]
                try:
                    text = reply[:-4].decode('utf-8', errors='strict')
                except UnicodeError as error:
                    raise NativeError('Invalid native reply encoding') from error
                if code != 0:
                    raise NativeError('Native command failed (status {}): {}'.format(code, text.strip()))
                if any(line.lstrip().startswith('%') for line in text.splitlines()):
                    raise NativeError('Native command returned an error: ' + text.strip())
                return text

    @contextmanager
    def session(self, daemon, budget):
        if daemon not in DAEMONS:
            raise NativeError('Unsupported daemon')
        deadline = self.clock() + float(budget)
        connection = self.socket_factory(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            connection.settimeout(min(self.timeout, float(budget)))
            connection.connect(str(self.directory / (daemon + '.vty')))
            self._command(connection, 'enable', deadline)
            yield connection, deadline
        except (OSError, UnicodeError) as error:
            raise NativeError('Native {} session unavailable: {}'.format(daemon, error)) from error
        finally:
            connection.close()

    def query(self, daemon, command='do write terminal', budget=5.0):
        with self.session(daemon, budget) as (connection, deadline):
            return self._command(connection, command, deadline)

    def apply(self, operations, budget=30.0):
        deadline = self.clock() + float(budget)
        for operation in operations:
            if not isinstance(operation, Operation) or not operation.commands:
                raise NativeError('Invalid native operation')
            remaining = deadline - self.clock()
            if remaining <= 0:
                raise NativeError('Native mutation deadline expired')
            with self.session(operation.daemon, remaining) as (connection, session_deadline):
                for command in operation.commands:
                    self._command(connection, command, min(deadline, session_deadline))
        return True


def complete_running_config(fragments):
    """Wrap successful framed daemon writer replies for the strict parser.

    FRR's vtysh performs the same outer `end` addition. Empty/error replies are
    rejected even when the transport reported success. Duplicate global headers
    are harmless; actual managed identities must remain unique in the parser.
    """
    if not fragments:
        raise NativeError('No native writer replies')
    lines = []
    for daemon, text in fragments:
        if daemon not in DAEMONS or not isinstance(text, str) or not text.strip():
            raise NativeError('Empty or invalid native writer reply')
        if any(line.lstrip().startswith('%') or 'Unknown command' in line
               for line in text.splitlines()):
            raise NativeError('Invalid native writer reply')
        lines.extend(line for line in text.splitlines() if line.strip() != 'end')
        lines.append('!')
    if not any(line.startswith('frr version ') for line in lines):
        raise NativeError('Native writer response lacks the FRR header')
    return '\n'.join(lines + ['end', ''])
