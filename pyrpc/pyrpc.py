import re
import time
from typing import Any

import msgpack
import serial

class Rpc:
    """Simple RPC client to an Arduino using MsgPack over serial.

    Protocol (aligned with your current main.py):
    - each call sends a sequence of concatenated MsgPack objects:
        1) procedure name (string)
        2) optional: arguments map (dict from str to value)
    - no automatic response handling: ``call()`` is fire-and-forget.
    """

    def __init__(self, device: str, baudrate: int = 115200, timeout: float = 1.0) -> None:
        """Initialize the serial connection.

        :param device: e.g. '/dev/ttyACM0'
        :param baudrate: typically 115200, must match the Arduino sketch
        :param timeout: read timeout in seconds
        """
        self._ser = serial.Serial(device, baudrate, timeout=timeout)
        time.sleep(0.1)
        
        # ask the server for all the available procedures
        self._help = self.call("help")

    def call(self, procedure: str, *args: Any) -> Any:
        """Send an RPC call and return the decoded response.

                We encode as a MsgPack sequence:
                - first object: procedure name (string)
                - following objects (if any): each positional argument value,
                    packed in the order provided via *args

        After writing, we read all available bytes from the serial
        (respecting the configured timeout) and decode all MsgPack
        objects found in the response buffer:
        - 0 objects  -> return None
        - 1 object   -> return that object
        - >1 objects -> return a list of objects
        """
        if not isinstance(procedure, str):
            raise TypeError("procedure must be a string")

        payload = msgpack.packb(procedure, use_bin_type=True)

        # Append each positional argument value as its own MsgPack object
        for value in args:
            payload += msgpack.packb(value, use_bin_type=True)

        self._ser.write(payload)
        self._ser.flush()

        return self._read_response()

    def _read_response(self) -> Any:
        """Read all available bytes from serial and decode MsgPack objects.

        Returns:
            - None if no decodable object is received
            - the single object if exactly one is present
            - a list of objects if multiple objects are present
        """
        raw = b""
        while True:
            # read at least 1 byte, or everything currently buffered
            to_read = self._ser.in_waiting or 1
            chunk = self._ser.read(to_read)
            if not chunk:
                break
            raw += chunk

        if not raw:
            return None

        unpacker = msgpack.Unpacker(raw=False)
        unpacker.feed(raw)
        objs = list(unpacker)

        if not objs:
            return None
        if len(objs) == 1:
            return objs[0]
        return objs

    def _format_help(self, raw: str) -> str:
        """Format the raw @entry/@brief/@return/@arg help string from the server.

        Expected raw format (one entry per line), for example::

            @entry f - @brief do nothing @return void

    Only @entry, @brief, @return and @arg are interpreted; anything
    else is ignored. The result is a multi-line, Python-style help
    string.
        """
        if not raw:
            return ""

        entries = []
        pattern = re.compile(
            r"@entry\s+(?P<name>\S+)"  # procedure name
            r"(?:\s*-\s*)?"            # optional dash separator
            r"(?:@brief\s+(?P<brief>[^@]+))?"  # brief until next tag
            r"(?:@return\s+(?P<ret>[^@]+))?"   # return until next tag
        )

        for line in raw.splitlines():
            line = line.strip()
            if not line:
                continue
            m = pattern.search(line)
            if not m:
                continue
            name = m.group("name").strip()
            brief = (m.group("brief") or "").strip()
            ret = (m.group("ret") or "").strip()

            args = []
            for am in re.finditer(r"@arg\s+(\S+)\s+([^@]+)", line):
                arg_name = am.group(1).strip()
                arg_desc = am.group(2).strip()
                args.append((arg_name, arg_desc))

            entries.append({"name": name, "brief": brief, "ret": ret, "args": args})

        if not entries:
            return raw

        out_lines = ["Available remote procedures:", ""]
        for e in entries:
            sig = f"{e['name']}()"
            out_lines.append(sig)
            if e["brief"]:
                out_lines.append(f"    {e['brief']}")
            if e.get("args"):
                out_lines.append("    Args:")
                for arg_name, arg_desc in e["args"]:
                    out_lines.append(f"        {arg_name}: {arg_desc}")
            if e["ret"]:
                out_lines.append(f"    Returns: {e['ret']}")
            out_lines.append("")

        return "\n".join(out_lines).rstrip() + "\n"

    @property
    def help(self) -> str:
        """Return the procedures available on the server, if any."""
        return self._format_help(self._help)

    @property
    def serial(self) -> serial.Serial:
        """Expose the underlying Serial object (for manual reads, etc.)."""
        return self._ser

    def close(self) -> None:
        """Close the serial port."""
        if self._ser and self._ser.is_open:
            self._ser.close()
