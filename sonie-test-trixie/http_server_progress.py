import http.server
import shutil
import sys
import os
import time
import socket
try:
    import portpicker
except ImportError:
    portpicker = None

class ProgressHandler(http.server.SimpleHTTPRequestHandler):
    def copyfile(self, source, outputfile):
        """Modified copyfile with progress indicator on stderr."""
        try:
            fs = os.fstat(source.fileno())
            total_size = fs.st_size
        except Exception:
            total_size = 0

        copied = 0
        start_time = time.time()

        while True:
            # 1MB chunks for balanced performance and progress granularity
            buf = source.read(1024*1024)
            if not buf:
                break
            outputfile.write(buf)
            copied += len(buf)

            if total_size > 0:
                percent = (copied * 100.0) / total_size
                # Using \r to overwrite the line in the console
                sys.stderr.write(f"\r[HTTP] Transfer progress: {copied}/{total_size} bytes ({percent:.1f}%)")
                sys.stderr.flush()

        sys.stderr.write("\n")
        sys.stderr.flush()

class DualStackServer(http.server.ThreadingHTTPServer):
    """
    Threaded server that defaults to IPv6 but typically supports IPv4 mapped addresses.
    Threading is REQUIRED for multi-stage boot environments to prevent lingering
    firmware connections from blocking OS-level requests.
    """
    address_family = socket.AF_INET6

    def server_bind(self):
        self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        super().server_bind()

def run_server(port=8000, host="", directory=""):
    if directory:
        print(f"Serving from directory {directory}")
        os.chdir(directory)

    if host and ":" in host:
        # Explicit IPv6 binding
        print(f"Binding to IPv6 {host}:{port}")
        httpd = DualStackServer((host, port), ProgressHandler)
    else:
        # IPv4 or default binding
        print(f"Binding to host {host}:{port}")
        httpd = http.server.ThreadingHTTPServer((host, port), ProgressHandler)

    httpd.serve_forever()

if __name__ == '__main__':
    requested_port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    if requested_port == 0 and portpicker:
        port = portpicker.pick_unused_port()
    else:
        port = requested_port

    run_server(
        port=port,
        host=sys.argv[2] if len(sys.argv) > 2 else "",
        directory=sys.argv[3] if len(sys.argv) > 3 else ""
    )
