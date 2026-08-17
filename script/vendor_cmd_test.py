#!/usr/bin/env python3
"""Exercise the AT+VENDOR_CMD pass-through across a real DG/HS pair.

Two things this is for:

  * Correctness at volume. Every command carries a counter in its payload, so the receiving
    side's events can be matched one-for-one against what was sent. That turns "it seems to
    work" into counted losses, duplicates and reorderings -- and it runs long enough to take
    the module's 8-bit sequence number past its wrap point, which a hand test never does.

  * Load. The +EVENT line is written to the AT UART with a BLOCKING transmit, from the
    wireless RX callback context. At 115200 baud a vendor event is roughly 2.3 ms spent
    inside that context. A sustained stream is therefore also an audio/link stress test, and
    the one the ODM is most likely to produce -- resending state periodically is exactly what
    this project's own guidance tells them to do. Run it with audio playing and watch for
    glitches, and watch LINK_WATCH on the other console if that board has one.

Both AT ports are needed: a command typed on one device surfaces as an event on the OTHER.

  python script/vendor_cmd_test.py --tx COM7 --rx COM9
  python script/vendor_cmd_test.py --tx COM7 --rx COM9 --count 300 --rate 10
  python script/vendor_cmd_test.py --tx COM7 --rx COM9 --ack
  python script/vendor_cmd_test.py --tx COM7 --rx COM9 --duration 300 --rate 20   # soak

Exit status is 0 only if nothing was lost, duplicated or reordered.
"""

import argparse
import re
import sys
import threading
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required:  pip install pyserial")

BAUD = 115200

# The module retransmits each best-effort command AT_VENDOR_TX_REPEAT (3) times and the
# receiver drops the duplicates, so one command must produce exactly one event. Seeing three
# is the signature of de-duplication having broken.
EXPECT_EVENTS_PER_COMMAND = 1

# A command occupies the vendor field for ~30 ms best effort, so the link carries roughly 33
# per second at most; the queue is 4 deep beyond that. Default well under it so a plain run
# measures correctness rather than back-pressure. Raise it to probe BUSY on purpose.
DEFAULT_RATE_HZ = 10.0

EVENT_RE = re.compile(r'\+EVENT:\s*VENDOR_CMD:(\d+)(?:,"([0-9A-Fa-f]*)")?')
ACK_RE = re.compile(r'\+EVENT:\s*VENDOR_CMD_(ACK|FAIL):(\d+)')


class Reader(threading.Thread):
    """Drain a port continuously and timestamp every line.

    A thread rather than polling because the receiving port also carries link events and
    crash-dump output, and anything we fail to drain promptly ends up in the driver buffer
    where its arrival time is lost -- which would make the latency numbers fiction.
    """

    def __init__(self, port, name):
        super().__init__(daemon=True)
        self.port = port
        self.name_ = name
        self.lines = []
        self.lock = threading.Lock()
        self.stop_flag = threading.Event()

    def run(self):
        buf = b""
        while not self.stop_flag.is_set():
            try:
                chunk = self.port.read(256)
            except (OSError, serial.SerialException) as exc:
                with self.lock:
                    self.lines.append((time.monotonic(), "<<port error: %s>>" % exc))
                return
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                text = raw.decode("ascii", "replace").strip()
                if text:
                    with self.lock:
                        self.lines.append((time.monotonic(), text))

    def take(self):
        with self.lock:
            out = self.lines[:]
            self.lines = []
        return out

    def stop(self):
        self.stop_flag.set()


def send_line(port, text):
    port.write((text + "\r\n").encode("ascii"))
    port.flush()


def await_response(reader, timeout=2.0):
    """Collect lines until the AT server's final OK / +CME ERROR for one command."""
    deadline = time.monotonic() + timeout
    collected = []
    while time.monotonic() < deadline:
        for _, line in reader.take():
            collected.append(line)
            if line == "OK" or line.startswith("+CME ERROR"):
                return line, collected
        time.sleep(0.002)
    return None, collected


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tx", required=True, help="AT port of the sending device")
    ap.add_argument("--rx", required=True, help="AT port of the receiving device")
    ap.add_argument("--id", type=int, default=1, help="vendor command id, 1-255 (default 1)")
    ap.add_argument("--count", type=int, default=300,
                    help="commands to send; >255 exercises the sequence wrap (default 300)")
    ap.add_argument("--duration", type=float, default=0.0,
                    help="soak for this many seconds instead of a fixed count")
    ap.add_argument("--rate", type=float, default=DEFAULT_RATE_HZ,
                    help="commands per second (default %.0f)" % DEFAULT_RATE_HZ)
    ap.add_argument("--ack", action="store_true",
                    help='send with "ACK" and check every command is confirmed')
    ap.add_argument("--settle", type=float, default=2.0,
                    help="seconds to keep listening after the last command (default 2)")
    args = ap.parse_args()

    if not 1 <= args.id <= 255:
        sys.exit("--id must be 1-255; 0 is reserved for \"no vendor command\"")

    period = 1.0 / args.rate if args.rate > 0 else 0.0

    print("tx=%s  rx=%s  id=%d  rate=%.1f/s  mode=%s"
          % (args.tx, args.rx, args.id, args.rate, "ACK" if args.ack else "best effort"))

    with serial.Serial(args.tx, BAUD, timeout=0.05) as tx, \
            serial.Serial(args.rx, BAUD, timeout=0.05) as rx:
        tx_reader, rx_reader = Reader(tx, "tx"), Reader(rx, "rx")
        tx_reader.start()
        rx_reader.start()
        time.sleep(0.3)
        tx_reader.take()
        rx_reader.take()

        sent = {}           # counter -> send timestamp
        busy = retried = 0
        errors = []
        acks, fails = [], []
        started = time.monotonic()
        counter = 0

        try:
            while True:
                if args.duration > 0:
                    if time.monotonic() - started >= args.duration:
                        break
                elif counter >= args.count:
                    break

                counter += 1
                # Two bytes of counter, so every command is individually identifiable and the
                # report can name what went missing instead of only counting.
                payload = "%04X" % (counter & 0xFFFF)
                cmd = 'AT+VENDOR_CMD=%d,"%s"%s' % (args.id, payload,
                                                   ',"ACK"' if args.ack else "")
                due = time.monotonic() + period

                send_line(tx, cmd)
                resp, lines = await_response(tx_reader)

                for line in lines:
                    m = ACK_RE.search(line)
                    if m:
                        (acks if m.group(1) == "ACK" else fails).append(int(m.group(2)))

                if resp is None:
                    errors.append((counter, "no response"))
                elif resp.startswith("+CME ERROR"):
                    if "BUSY" in resp:
                        # Expected back-pressure, not a defect: the host is outrunning the
                        # 10 ms link. Back off and resend so the counter stays contiguous.
                        busy += 1
                        counter -= 1
                        time.sleep(0.05)
                        retried += 1
                        continue
                    errors.append((counter, resp))
                else:
                    sent[counter] = time.monotonic()

                remaining = due - time.monotonic()
                if remaining > 0:
                    time.sleep(remaining)
        except KeyboardInterrupt:
            print("\ninterrupted; reporting what completed so far")

        elapsed = time.monotonic() - started
        time.sleep(args.settle)

        # ACKs can still be arriving after the last command.
        for _, line in tx_reader.take():
            m = ACK_RE.search(line)
            if m:
                (acks if m.group(1) == "ACK" else fails).append(int(m.group(2)))

        received = []
        strays = []
        for ts, line in rx_reader.take():
            m = EVENT_RE.search(line)
            if m:
                received.append((ts, int(m.group(1)), (m.group(2) or "").upper()))
            elif line.startswith("+EVENT:"):
                strays.append(line)

        tx_reader.stop()
        rx_reader.stop()

    # ---- report ----------------------------------------------------------------
    print("\n%d commands accepted in %.1f s (%.1f/s effective)"
          % (len(sent), elapsed, len(sent) / elapsed if elapsed else 0.0))
    if busy:
        print("  BUSY back-pressure: %d (retried %d) -- expected if --rate is high" % (busy, retried))

    seen = {}
    wrong_id = 0
    undecodable = []
    for ts, vid, payload in received:
        if vid != args.id:
            wrong_id += 1
            continue
        try:
            n = int(payload, 16)
        except ValueError:
            undecodable.append(payload)
            continue
        seen.setdefault(n, []).append(ts)

    expected = set(sent)
    got = set(seen)
    missing = sorted(expected - got)
    unexpected = sorted(got - expected)
    duplicated = sorted(n for n, times in seen.items() if len(times) > EXPECT_EVENTS_PER_COMMAND)

    order = [n for _, vid, p in received if vid == args.id
             for n in [int(p, 16)] if n in expected]
    reordered = sum(1 for a, b in zip(order, order[1:]) if b < a)

    latencies = sorted((times[0] - sent[n]) * 1000.0
                       for n, times in seen.items() if n in sent)

    print("  events received : %d" % len(received))
    print("  matched         : %d" % len(got & expected))
    print("  MISSING         : %d %s" % (len(missing), _sample(missing)))
    print("  DUPLICATED      : %d %s" % (len(duplicated), _sample(duplicated)))
    print("  out of order    : %d" % reordered)
    if unexpected:
        print("  unexpected ids  : %d %s" % (len(unexpected), _sample(unexpected)))
    if wrong_id:
        print("  wrong vendor id : %d" % wrong_id)
    if undecodable:
        print("  bad payloads    : %d %s" % (len(undecodable), _sample(undecodable)))
    if latencies:
        print("  latency ms      : min %.0f  median %.0f  p95 %.0f  max %.0f"
              % (latencies[0], latencies[len(latencies) // 2],
                 latencies[int(len(latencies) * 0.95)], latencies[-1]))
    if args.ack:
        print("  VENDOR_CMD_ACK  : %d" % len(acks))
        print("  VENDOR_CMD_FAIL : %d" % len(fails))
    if strays:
        print("  other +EVENT lines on rx (%d), first few:" % len(strays))
        for line in strays[:5]:
            print("      %s" % line)
    if errors:
        print("  command errors  : %d, first few:" % len(errors))
        for n, why in errors[:5]:
            print("      #%d %s" % (n, why))

    ok = not (missing or duplicated or reordered or unexpected or wrong_id
              or undecodable or errors)
    if args.ack:
        # Every accepted command must resolve one way or the other, and a FAIL while the link
        # is up is itself a finding.
        if len(acks) != len(sent) or fails:
            ok = False
    print("\n%s" % ("PASS" if ok else "FAIL"))

    if duplicated:
        print("  duplicates mean the receiver stopped de-duplicating on (id, seq): the module"
              "\n  sends each best-effort command 3 times, so expect ~3x, not 2x.")
    if missing and not duplicated:
        print("  losses are expected for best effort at the edge of range, and NOT expected"
              "\n  with --ack while the link is up. Re-run with --ack to tell the two apart.")

    return 0 if ok else 1


def _sample(items, n=8):
    if not items:
        return ""
    head = ", ".join(str(x) for x in items[:n])
    return "[%s%s]" % (head, ", ..." if len(items) > n else "")


if __name__ == "__main__":
    sys.exit(main())
