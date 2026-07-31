#!/usr/bin/env python3
"""AT-command RX stress / corruption-rate tester.

Purpose
-------
Reproduce and quantify the "UWB-connected -> BT->MCU AT command dropped bytes"
problem on the bench. It spams an AT command at the module's expansion UART and
classifies every reply using the firmware's own echo probe (commit 6e2019f):

    recognised as AT  -> "+DBG OK:   [<exact line the MCU assembled>]"
    NOT starting "AT" -> "+DBG RECV: [<garbled line>]"  (+ HELP dump)

So per command we simply ask: did we get back "+DBG OK: [<the command we sent>]"
verbatim? Anything else means bytes were dropped/corrupted on the MCU's RX path.

Method
------
Run it TWICE and compare:
  A) baseline  : UWB NOT connected            -> expect ~0% corruption
  B) under load: UWB connected (+ audio best) -> expect measurable corruption

If A is clean and B corrupts, and the rate rises with command length / radio
load, that confirms the priority-15 byte-IRQ RX starvation (software), not a
one-off. The B-rate is also the pass/fail metric for the RX-DMA fix (-> 0%).

Wiring reminder (MCU RX is where the PC's TX must go):
  u5a5  USART2  : MCU TX=PA2  RX=PA3   (adapter TX -> PA3)
  u535  LPUART1 : MCU TX=PA3  RX=PA2   (adapter TX -> PA2)   [TX/RX swapped]

Usage
-----
  python at_rx_stress.py --port COM7
  python at_rx_stress.py --port COM7 --count 500 --gap 0.08
  python at_rx_stress.py --port COM7 --sweep          # vary command length
  python at_rx_stress.py --list                       # list serial ports

Needs pyserial:  pip install pyserial
"""
import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial not installed. Run:  pip install pyserial")


# Commands of increasing length for --sweep. Longer = more bytes = more chances
# to hit a busy ISR window, so corruption rate should climb down this list.
SWEEP_CMDS = [
    "AT",
    "AT+PING",
    "AT+VER?",
    "AT+UWB_PAIR",
    "AT+UWB_CONN_STATUS?",
    "AT+UWB_GET_ROLE?",
]

# Classification of a single command round-trip.
CLEAN = "clean"          # +DBG OK: [<cmd>]  exactly
CORRUPT = "corrupt"      # echo present but content != cmd (dropped/garbled bytes)
TIMEOUT = "timeout"      # no echo line seen within --timeout


def find_echo(ser, cmd, deadline, samples):
    """Read lines until we see the module's echo probe or the deadline passes.

    Returns one of CLEAN / CORRUPT / TIMEOUT. Non-echo lines (OK, +UWB...,
    HELP list, periodic +CRASH_DUMP, etc.) are ignored on purpose.
    """
    while time.monotonic() < deadline:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("ascii", errors="replace").strip()

        inner = None
        if line.startswith("+DBG OK: [") and line.endswith("]"):
            inner = line[len("+DBG OK: ["):-1]
        elif line.startswith("+DBG RECV: [") and line.endswith("]"):
            inner = line[len("+DBG RECV: ["):-1]
        else:
            continue  # not an echo line -> keep looking

        if inner == cmd:
            return CLEAN
        # Echo came back but not equal to what we sent -> corrupted RX.
        if len(samples) < 20:
            samples.append(inner)
        return CORRUPT

    return TIMEOUT


def run_batch(ser, cmd, count, gap, timeout):
    """Send `cmd` `count` times; return (counts dict, corrupt-payload samples)."""
    counts = {CLEAN: 0, CORRUPT: 0, TIMEOUT: 0}
    samples = []
    payload = (cmd + args.eol).encode("ascii")

    for i in range(1, count + 1):
        ser.reset_input_buffer()
        ser.write(payload)
        ser.flush()
        result = find_echo(ser, cmd, time.monotonic() + timeout, samples)
        counts[result] += 1

        if i % 25 == 0 or i == count:
            bad = counts[CORRUPT] + counts[TIMEOUT]
            print(f"  [{i:>4}/{count}] corrupt={counts[CORRUPT]} "
                  f"timeout={counts[TIMEOUT]}  bad-rate={bad / i * 100:5.1f}%",
                  end="\r", flush=True)
        if gap:
            time.sleep(gap)

    print()  # end the \r line
    return counts, samples


def summarize(cmd, counts, samples):
    total = sum(counts.values())
    bad = counts[CORRUPT] + counts[TIMEOUT]
    print(f"\n  cmd = {cmd!r}  ({len(cmd)} bytes payload, "
          f"{len(cmd) + len(args.eol)} on the wire)")
    print(f"    clean   : {counts[CLEAN]:>5}")
    print(f"    corrupt : {counts[CORRUPT]:>5}")
    print(f"    timeout : {counts[TIMEOUT]:>5}")
    print(f"    TOTAL   : {total:>5}   ->  bad-rate = "
          f"{(bad / total * 100) if total else 0:.2f}%")
    if samples:
        print("    corrupt samples (what the MCU actually assembled):")
        for s in samples:
            print(f"        [{s}]")
    return bad, total


def main():
    if args.list:
        ports = list(list_ports.comports())
        if not ports:
            print("No serial ports found.")
        for p in ports:
            print(f"  {p.device:<12} {p.description}")
        return

    if not args.port:
        sys.exit("Specify --port (or --list to see available ports).")

    print(f"Opening {args.port} @ {args.baud} 8N1 ...")
    ser = serial.Serial(args.port, args.baud, timeout=args.readtimeout)
    time.sleep(0.2)
    ser.reset_input_buffer()

    cmds = SWEEP_CMDS if args.sweep else [args.cmd]
    print(f"Sending each command x{args.count}, gap={args.gap*1000:.0f}ms, "
          f"per-cmd timeout={args.timeout*1000:.0f}ms")
    print("Tip: run once with UWB DISCONNECTED (baseline), once CONNECTED (+audio).")
    print("=" * 64)

    grand_bad = grand_total = 0
    try:
        for cmd in cmds:
            print(f"\n>>> {cmd}")
            counts, samples = run_batch(ser, cmd, args.count, args.gap, args.timeout)
            bad, total = summarize(cmd, counts, samples)
            grand_bad += bad
            grand_total += total
    except KeyboardInterrupt:
        print("\n\nInterrupted by user; partial results above.")
    finally:
        ser.close()

    if len(cmds) > 1 and grand_total:
        print("\n" + "=" * 64)
        print(f"OVERALL bad-rate across all commands: "
              f"{grand_bad / grand_total * 100:.2f}%  "
              f"({grand_bad}/{grand_total})")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description="AT-command RX corruption-rate tester",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    ap.add_argument("--port", help="serial port, e.g. COM7 or /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200, help="baud rate")
    ap.add_argument("--cmd", default="AT+UWB_CONN_STATUS?",
                    help="AT command to hammer")
    ap.add_argument("--count", type=int, default=200,
                    help="how many times to send the command")
    ap.add_argument("--gap", type=float, default=0.1,
                    help="seconds between commands")
    ap.add_argument("--timeout", type=float, default=1.0,
                    help="seconds to wait for the echo of each command")
    ap.add_argument("--readtimeout", type=float, default=0.1,
                    help="pyserial readline() timeout (keep small)")
    ap.add_argument("--eol", default="\r\n",
                    help=r"line ending appended to each command (default \r\n)")
    ap.add_argument("--sweep", action="store_true",
                    help="sweep several commands of increasing length")
    ap.add_argument("--list", action="store_true",
                    help="list serial ports and exit")
    args = ap.parse_args()
    # allow literal \r \n on the command line
    args.eol = args.eol.encode().decode("unicode_escape")
    main()
