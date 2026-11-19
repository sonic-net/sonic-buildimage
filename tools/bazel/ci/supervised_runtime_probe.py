"""Check the installed sidecar payloads and watchdog HTTP endpoints in a selected VS archive."""

import http.client
import json
import os
import subprocess
import time
from pathlib import Path
import sys


def main(image):
    assert os.environ["IMAGE_VERSION"]
    assert os.environ["DEBIAN_FRONTEND"] == "noninteractive"
    assert Path("/usr/local/bin/supervisord").is_file()
    assert Path("/etc/supervisor/conf.d/supervisord.conf").is_file()
    if image == "docker-gnmi-sidecar":
        from sonic_py_common import sidecar_common
        assert os.environ["IS_V1_ENABLED"] == "false"
        assert os.environ["CONTAINER_CHECKER_SYNC_ENABLED"] == "false"
        for path in ("/usr/bin/systemd_stub.py", "/usr/share/sonic/scripts/k8s_pod_control.sh",
                     "/usr/share/sonic/systemd_scripts/gnmi_v1.sh"):
            assert os.access(path, os.X_OK), path
        assert "gnmi-sidecar#%syslogtag%" in Path("/etc/rsyslog.conf").read_text()
    elif image == "docker-restapi-sidecar":
        from sonic_py_common import sidecar_common
        assert os.environ["IS_V1_ENABLED"] == "false"
        scripts = Path("/usr/share/sonic/systemd_scripts")
        for release in ("202311", "202405", "202411", "202505", "202511"):
            assert (scripts / f"restapi.service_{release}").is_file()
            assert os.access(scripts / "v1" / f"restapi.sh_{release}", os.X_OK)
            assert os.access(scripts / f"container_checker_{release}", os.X_OK)
        assert os.access("/usr/bin/systemd_stub.py", os.X_OK)
        assert Path("/etc/supervisor/critical_processes").is_file()
        assert "restapi-sidecar#%syslogtag%" in Path("/etc/rsyslog.conf").read_text()
    elif image == "docker-gnmi-watchdog":
        name, port = "gnmi_watchdog", 51000
        executable = "/usr/bin/" + name
        with subprocess.Popen([executable], stdout=sys.stderr, stderr=sys.stderr) as server:
            try:
                for attempt in range(100):
                    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=2)
                    try:
                        conn.request("GET", "/")
                        response = conn.getresponse()
                        result = json.loads(response.read())
                        break
                    except ConnectionRefusedError:
                        if server.poll() is not None or attempt == 99:
                            raise
                        time.sleep(0.05)
                    finally:
                        conn.close()
                if name == "gnmi_watchdog":
                    assert response.status == 200 and result == {"gnmi_status": "OK"}, result
            finally:
                server.terminate()
    else:
        raise ValueError(image)
    print(json.dumps({"image": image, "status": "passed"}))


if __name__ == "__main__":
    main(sys.argv[1])
