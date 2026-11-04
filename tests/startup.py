#!/usr/bin/env python3
"""Optional integration check: supervise real subprocesses, without a Wayland session."""
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "dwl-startup"


class Startup(unittest.TestCase):
    def wait_for(self, predicate):
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if predicate():
                return
            time.sleep(0.01)
        self.fail("startup condition did not become true")

    def scenario(self, bar, terminate=False):
        with tempfile.TemporaryDirectory(prefix="dwl-startup-") as directory:
            root = Path(directory)
            for name in ("bar", "notify", "exiting-bar"):
                child = root / name
                child.write_text(
                    "#!/usr/bin/env python3\n"
                    "import os, pathlib, signal, sys\n"
                    "root = pathlib.Path(os.environ['DWL_TEST_DIR'])\n"
                    "name = pathlib.Path(__file__).name\n"
                    "(root / (name + '.pid')).write_text(str(os.getpid()))\n"
                    "if name == 'notify':\n"
                    "    while True: signal.pause()\n"
                    "elif name == 'bar':\n"
                    "    with (root / 'status').open('w') as out:\n"
                    "        for line in sys.stdin:\n"
                    "            out.write(line); out.flush()\n"
                )
                child.chmod(0o755)
            env = dict(os.environ, DWL_TEST_DIR=directory,
                       DWL_BAR=str(root / bar), DWL_NOTIFY=str(root / "notify"))
            with subprocess.Popen(["sh", str(SCRIPT)], env=env,
                                  stdin=subprocess.PIPE, stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL) as proc:
                try:
                    self.wait_for(lambda: (root / "notify.pid").exists())
                    if bar != "missing":
                        self.wait_for(lambda: (root / (bar + ".pid")).exists())
                    proc.stdin.write(b"HEADLESS-1 tags 1 1 1 0\n")
                    proc.stdin.flush()
                    if bar == "bar":
                        self.wait_for(lambda: (root / "status").exists() and
                                      "tags 1 1 1 0" in (root / "status").read_text())
                    # Enough data to detect a missing drain through a blocked pipe.
                    if terminate:
                        self.assertIsNone(proc.poll())
                        proc.terminate()
                        proc.communicate(timeout=5)
                    else:
                        proc.communicate(b"HEADLESS-1 layout []=\n" * 10000, timeout=5)
                    self.assertEqual(proc.returncode, 0)
                finally:
                    if proc.poll() is None:
                        proc.send_signal(signal.SIGTERM)
                        proc.wait(timeout=5)
                for pidfile in root.glob("*.pid"):
                    with self.assertRaises(ProcessLookupError):
                        os.kill(int(pidfile.read_text()), 0)

    def test_status_and_cleanup(self):
        self.scenario("bar")

    def test_missing_bar_drains(self):
        self.scenario("missing")

    def test_exited_bar_drains(self):
        self.scenario("exiting-bar")

    def test_termination_stops_children(self):
        self.scenario("bar", terminate=True)


if __name__ == "__main__":
    unittest.main()
