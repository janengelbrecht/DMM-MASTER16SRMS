#!/usr/bin/env python3
"""
dmm_master.py — SCPI-klient til DMM-MASTER 16SRMS.

Understøtter:
  --idn         Læs instrumentets identifikation
  --measure     Tag en enkelt måling
  --stream N    Stream N målinger til stdout (CSV)
  --log FIL     Log målinger til CSV-fil
  --command C   Send vilkårlig SCPI-kommando

Brug:
  python dmm_master.py --port /dev/ttyACM0 --measure
  python dmm_master.py --port COM3 --stream 100 --log data.csv

Kræver: pyserial (se requirements.txt)
"""

import argparse
import csv
import sys
import time

import serial


def open_port(port: str, baud: int = 115200) -> serial.Serial:
    """Åbn seriel port med de parametre DMM-MASTER kræver."""
    return serial.Serial(port, baudrate=baud, timeout=2.0)


def send_command(dmm: serial.Serial, cmd: str) -> str:
    """Send én SCPI-kommando og returnér svaret (uden trailing newline)."""
    dmm.write((cmd + "\n").encode("ascii"))
    dmm.flush()
    response = dmm.readline().decode("ascii", errors="replace").strip()
    return response


def cmd_idn(dmm: serial.Serial) -> None:
    """Læs *IDN? og print resultatet."""
    print(send_command(dmm, "*IDN?"))


def cmd_measure(dmm: serial.Serial) -> None:
    """Tag én måling via READ? og print værdien."""
    value = send_command(dmm, "READ?")
    print(value)


def cmd_stream(dmm: serial.Serial, count: int, logfile: str | None) -> None:
    """Stream N målinger som CSV og log dem valgfrit til fil."""
    writer = None
    log_handle = None
    if logfile:
        log_handle = open(logfile, "w", newline="")
        writer = csv.writer(log_handle)
        writer.writerow(["timestamp_ms", "value"])

    print("timestamp_ms,value")
    for i in range(count):
        timestamp_ms = int(time.time() * 1000)
        value = send_command(dmm, "READ?")
        print(f"{timestamp_ms},{value}")
        if writer:
            writer.writerow([timestamp_ms, value])

    if log_handle:
        log_handle.close()


def cmd_send(dmm: serial.Serial, command: str) -> None:
    """Send vilkårlig SCPI-kommando og print evt. svar."""
    response = send_command(dmm, command)
    if response:
        print(response)


def main() -> int:
    parser = argparse.ArgumentParser(description="DMM-MASTER 16SRMS SCPI-klient")
    parser.add_argument("--port", required=True, help="Seriel port (fx /dev/ttyACM0 eller COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baudrate (default: 115200)")
    parser.add_argument("--idn", action="store_true", help="Læs *IDN?")
    parser.add_argument("--measure", action="store_true", help="Tag én måling")
    parser.add_argument("--stream", type=int, metavar="N", help="Stream N målinger")
    parser.add_argument("--log", metavar="FILE", help="Log stream til CSV-fil")
    parser.add_argument("--command", metavar="CMD", help="Send vilkårlig SCPI-kommando")
    args = parser.parse_args()

    try:
        with open_port(args.port, args.baud) as dmm:
            if args.idn:
                cmd_idn(dmm)
            if args.measure:
                cmd_measure(dmm)
            if args.stream:
                cmd_stream(dmm, args.stream, args.log)
            if args.command:
                cmd_send(dmm, args.command)
    except serial.SerialException as exc:
        print(f"Fejl ved åbning af port: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nAfbrudt af bruger.", file=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main())