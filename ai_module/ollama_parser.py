"""Local Ollama backend: instruction in, validated command data out."""

import json
import os
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

# Importing these helpers does not call OpenAI or read an API key.
from llm_parser import SCHEMA, INSTRUCTIONS, _unique_object, _reject_constant, validate_command


def _decode_response(response):
    if (not isinstance(response, dict) or response.get("done") is not True
            or response.get("done_reason") != "stop"):
        raise ValueError("Ollama response did not finish normally.")
    message = response.get("message")
    if (not isinstance(message, dict) or message.get("role") != "assistant"
            or message.get("tool_calls") or not isinstance(message.get("content"), str)):
        raise ValueError("Malformed Ollama message.")
    try:
        envelope = json.loads(message["content"], object_pairs_hook=_unique_object,
                              parse_constant=_reject_constant)
    except json.JSONDecodeError:
        raise ValueError("Ollama returned invalid JSON.") from None
    if not isinstance(envelope, dict) or set(envelope) != {"result"}:
        raise ValueError("Ollama returned an invalid response wrapper.")
    if envelope["result"] is None:
        raise ValueError("Unsupported or ambiguous instruction.")
    return validate_command(envelope["result"])


def parse_instruction(text: str) -> dict:
    """Return one command; raise ValueError or RuntimeError on failure."""
    if not isinstance(text, str) or not text.strip():
        raise ValueError("Instruction must be non-empty text.")
    host = os.environ.get("OLLAMA_HOST", "http://localhost:11434").strip().rstrip("/")
    model = os.environ.get("OLLAMA_MODEL", "qwen3.5:4b").strip()
    if not host.startswith(("http://", "https://")) or not model:
        raise RuntimeError("Set OLLAMA_HOST to an HTTP URL and OLLAMA_MODEL to a model name.")
    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": INSTRUCTIONS +
             "\nColor moods may be interpreted as RGB colors. Interpret rendering "
             "mode requests by their meaning, not by exact wording. In this application, "
             "normal rendering means solid, filled triangles with wireframe disabled. "
             "Requests to render normally again, return to normal rendering, or use solid "
             "or filled rendering map to set_wireframe with enabled false, not reset_scene. "
             "These specify a target mode and do not require knowing the current state. "
             "Requests to show triangle edges, outlines, or wireframe map to set_wireframe "
             "with enabled true. Apply these meanings to paraphrases as well. A mere "
             "mention of edges or normal rendering is not necessarily a request to change "
             "mode. Still reject ambiguous toggles, conflicting or multiple actions, "
             "and unsupported requests."
             "\nReturn JSON matching this schema: " + json.dumps(SCHEMA)},
            {"role": "user", "content": text},
        ],
        "format": SCHEMA,
        "stream": False,
        "think": False,
        "options": {"temperature": 0, "num_predict": 256},
    }
    request = Request(host + "/api/chat", data=json.dumps(payload).encode("utf-8"),
                      headers={"Content-Type": "application/json"}, method="POST")
    try:
        with urlopen(request, timeout=120) as response:
            result = json.load(response)
    except HTTPError as error:
        raise RuntimeError(
            f"Ollama HTTP {error.code}. Check that the model is installed and "
            "your Ollama version supports this request."
        ) from None
    except (URLError, OSError):
        raise RuntimeError("Could not reach Ollama (server unavailable or timeout).") from None
    except (ValueError, UnicodeError):
        raise RuntimeError("Ollama returned unreadable JSON.") from None
    return _decode_response(result)
