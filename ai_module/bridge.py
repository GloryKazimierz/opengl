"""Write validated background commands to the renderer's file mailbox."""

import os
from pathlib import Path
import tempfile

from llm_parser import validate_command


BRIDGE_PATH = Path(__file__).resolve().parent / "renderer_command.txt"


def write_command(command: dict) -> Path:
    """Validate, check bridge support, then publish a complete command file."""
    command = validate_command(command)
    if command["command"] != "set_background":
        raise ValueError(
            f"Bridge does not support {command['command']} yet; only set_background is supported."
        )
    line = "set_background " + " ".join(str(v) for v in command["color"]) + "\n"
    temporary = None
    try:
        # Same directory keeps replacement on the same filesystem. Close the
        # temporary file before replacing, which is necessary on Windows.
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="ascii", newline="\n", dir=BRIDGE_PATH.parent,
            prefix=".renderer_command-", suffix=".tmp", delete=False,
        ) as stream:
            temporary = Path(stream.name)
            stream.write(line)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, BRIDGE_PATH)
        return BRIDGE_PATH
    except OSError as error:
        raise RuntimeError(f"Could not write bridge file: {error}") from None
    finally:
        if temporary is not None:
            try:
                temporary.unlink()
            except FileNotFoundError:
                pass
