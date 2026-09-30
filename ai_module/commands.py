"""Translate a small set of English instructions into renderer command data."""

import re


COLORS = {
    "black": [0.0, 0.0, 0.0],
    "white": [1.0, 1.0, 1.0],
    "red": [1.0, 0.0, 0.0],
    "green": [0.0, 1.0, 0.0],
    "blue": [0.0, 0.0, 1.0],
    "yellow": [1.0, 1.0, 0.0],
    "orange": [1.0, 0.5, 0.0],
    "purple": [0.5, 0.0, 0.5],
    "gray": [0.5, 0.5, 0.5],
    "grey": [0.5, 0.5, 0.5],
    "cyan": [0.0, 1.0, 1.0],
    "magenta": [1.0, 0.0, 1.0],
    "light blue": [0.5, 0.5, 1.0],
    "dark blue": [0.0, 0.0, 0.5],
    "light green": [0.5, 1.0, 0.5],
    "dark green": [0.0, 0.5, 0.0],
    "light red": [1.0, 0.5, 0.5],
    "dark red": [0.5, 0.0, 0.0],
}

CHINESE_COLORS = {
    "红色": "red", "绿色": "green", "蓝色": "blue", "黄色": "yellow",
    "橙色": "orange", "紫色": "purple", "白色": "white", "黑色": "black",
    "灰色": "gray", "青色": "cyan", "品红色": "magenta",
    "浅蓝色": "light blue", "深蓝色": "dark blue",
    "浅绿色": "light green", "深绿色": "dark green",
    "浅红色": "light red", "深红色": "dark red",
}


def parse_instruction(text: str) -> dict:
    """Return one command dictionary, or raise ValueError for unsupported text.

    This is the interface a future LLM parser can implement. Callers need not
    know how the instruction is interpreted; command names and fields stay fixed.
    """
    # Ignore case, repeated whitespace, and trailing sentence punctuation.
    instruction = " ".join(text.lower().split()).rstrip(".!?。！？")

    match = re.fullmatch(
        r"(?:change|set) (?:the )?background to (black|white|red|green|blue)",
        instruction,
    )
    if match:
        # Copy the list so callers cannot accidentally change our color table.
        return {"command": "set_background", "color": COLORS[match[1]].copy()}

    # Match a small set of whole-sentence patterns; unknown colors stay invalid.
    match = re.fullmatch(
        r"(?:make (?:(?:the )?(?:object|triangle)|it)|"
        r"(?:change|set) (?:the )?(?:object|triangle)(?: color)? to|"
        r"color (?:the )?(?:object|triangle)) (.+)", instruction,
    ) or re.fullmatch(r"把(?:物体|三角形)(?:颜色)?(?:改成|变成|设为)(.+)", instruction)
    if match:
        color_name = CHINESE_COLORS.get(match[1], match[1])
        if color_name in COLORS:
            r, g, b = COLORS[color_name]
            return {"command": "set_object_color", "r": r, "g": g, "b": b}

    if instruction in {
        "switch to wireframe mode",
        "enable wireframe",
        "turn on wireframe",
        "turn wireframe on",
        "enable wireframe mode",
        "wireframe on",
        "show wireframe",
        "use wireframe mode",
        "打开线框",
        "打开线框模式",
        "开启线框模式",
    }:
        return {"command": "set_wireframe", "enabled": True}

    if instruction in {
        "switch to fill mode",
        "disable wireframe",
        "turn off wireframe",
        "turn wireframe off",
        "disable wireframe mode",
        "wireframe off",
        "solid mode",
        "go back to solid rendering",
        "关闭线框",
        "关闭线框模式",
    }:
        return {"command": "set_wireframe", "enabled": False}

    if instruction in {"reset scene", "reset the scene"}:
        return {"command": "reset_scene"}

    raise ValueError(
        "Unsupported instruction. Try 'change the background to blue', "
        "'make the object red', 'switch to wireframe mode', 'disable wireframe', "
        "or 'reset the scene'."
    )
