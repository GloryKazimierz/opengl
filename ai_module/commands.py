"""Translate a small set of English instructions into renderer command data."""

import re


COLORS = {
    "black": [0.0, 0.0, 0.0],
    "white": [1.0, 1.0, 1.0],
    "red": [1.0, 0.0, 0.0],
    "green": [0.0, 1.0, 0.0],
    "blue": [0.0, 0.0, 1.0],
}


def parse_instruction(text: str) -> dict:
    """Return one command dictionary, or raise ValueError for unsupported text.

    This is the interface a future LLM parser can implement. Callers need not
    know how the instruction is interpreted; command names and fields stay fixed.
    """
    # Ignore case, repeated whitespace, and trailing sentence punctuation.
    instruction = " ".join(text.lower().split()).rstrip(".!?")

    match = re.fullmatch(
        r"(?:change|set) (?:the )?background to (black|white|red|green|blue)",
        instruction,
    )
    if match:
        # Copy the list so callers cannot accidentally change our color table.
        return {"command": "set_background", "color": COLORS[match[1]].copy()}

    if instruction in {
        "switch to wireframe mode",
        "enable wireframe",
        "turn on wireframe",
    }:
        return {"command": "set_wireframe", "enabled": True}

    if instruction in {
        "switch to fill mode",
        "disable wireframe",
        "turn off wireframe",
    }:
        return {"command": "set_wireframe", "enabled": False}

    if instruction in {"reset scene", "reset the scene"}:
        return {"command": "reset_scene"}

    raise ValueError(
        "Unsupported instruction. Try 'change the background to blue', "
        "'switch to wireframe mode', 'disable wireframe', or 'reset the scene'."
    )
