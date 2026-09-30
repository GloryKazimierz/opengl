# Standalone instruction parser

This prototype converts English instructions into structured command data using
rules (default), OpenAI, or local Ollama. By default it prints JSON only. Optional
`--bridge` publishes a background command for the renderer to consume through
a file. It never calls OpenGL or contacts the renderer process directly.
It uses Python 3 and its standard library, with no
packages to install.

## Files

- `commands.py`: contains the named RGB colors and `parse_instruction(text)`.
  This function is the boundary between natural language and command data.
- `main.py`: reads one command-line argument, calls the parser, and prints JSON.
  Unsupported instructions go to standard error and produce exit status 1.
- `README.md`: explains the interface, supported inputs, and usage.
- `gui.py`: Tkinter input/status window using the existing Ollama parser and bridge.
- `bridge.py`: validates commands and atomically publishes supported background
  commands to `renderer_command.txt` beside this file.
- `llm_parser.py`: sends an OpenAI API request with a strict schema and validates
  the response locally before returning a command.
- `ollama_parser.py`: calls the local Ollama server and returns the same validated
  commands. Reuses schema/validation helpers without calling OpenAI.

## Command format

Each successful call returns one Python dictionary. The command-line interface
serializes that dictionary as a JSON object.

| Command | Fields | Intended meaning for a future consumer |
| --- | --- | --- |
| `set_background` | `color`: three RGB numbers from 0.0 to 1.0 | Set background color |
| `set_wireframe` | `enabled`: boolean | Explicitly enable or disable wireframe |
| `reset_scene` | No additional fields | Request a scene reset |

Examples:

```json
{"command": "set_background", "color": [0.0, 0.0, 1.0]}
{"command": "set_wireframe", "enabled": true}
{"command": "set_wireframe", "enabled": false}
{"command": "reset_scene"}
```

These are four separate command examples, not one JSON document. Reset behavior
is not implemented here; a future renderer consumer must define its defaults.

## How rule-based parsing works

1. Convert text to lowercase, collapse repeated whitespace, and remove trailing
   sentence punctuation (`.`, `!`, or `?`).
2. Match the entire instruction against a small set of supported patterns.
3. Return a command dictionary, or raise `ValueError` if nothing matches.

Supported phrases:

- `change the background to blue` or `set background to blue`: replace `blue`
  with `black`, `white`, `red`, or `green` as well. `the` is optional.
- `switch to wireframe mode`, `enable wireframe`, or `turn on wireframe`.
- `switch to fill mode`, `disable wireframe`, or `turn off wireframe`.
- `reset scene` or `reset the scene`.

One instruction produces one command. Arbitrary paraphrases, multiple commands,
custom RGB values, and unsupported colors are rejected rather than guessed.
For example, `do not reset the scene` is rejected, not interpreted as a reset.

## Run examples

From `C:\Users\Wang\Desktop\project`, use PowerShell:

```powershell
python -B ai_module/main.py "change the background to blue"
python -B ai_module/main.py "switch to wireframe mode"
python -B ai_module/main.py "disable wireframe"
python -B ai_module/main.py "reset the scene"
```

The outputs match the four JSON examples above. `-B` prevents Python from writing
bytecode cache files. No CMake configuration or renderer build is needed.

An unsupported input demonstrates the error path:

```powershell
python -B ai_module/main.py "make everything sparkle"
```

## Replacing the parser later

The public interface is:

```python
parse_instruction(text: str) -> dict
```

It returns one of the documented command dictionaries and raises `ValueError`
for unsupported instructions. It has no rendering side effects.

Both backends preserve this function signature and command format. The default
rule-based backend makes no network or model calls. The LLM backend additionally
raises RuntimeError for configuration, network, and API failures.

All module code lives inside `ai_module`. The optional file bridge is the only
renderer communication; there is no shared OpenGL state or dependency on the
existing renderer build system.

## Use the LLM backend

Set environment variables in PowerShell 7. Masked input keeps the key out of
the typed command history; the module never saves it to a file:

```powershell
$env:OPENAI_API_KEY = Read-Host 'OpenAI API key' -MaskInput
$env:OPENAI_MODEL = Read-Host 'Model ID supporting Responses and Structured Outputs'
python -B ai_module/main.py --parser llm "Please make the background blue"
python -B ai_module/main.py --parser llm "Show the scene as wireframe"
python -B ai_module/main.py --parser llm "Reset the scene"
```

Use a model available to your API account that supports the supplied schema.
No model or API key is hard-coded. Unsupported models produce an error; the
module never falls back to unconstrained output or to rule-based parsing.
No SDK installation or .env file is needed.

LLM mode sends the instruction and parsing prompt/schema to OpenAI in one HTTPS
request, with a 60-second timeout and `store: false`. Normal API charges apply.
Rule mode works without credentials:

```powershell
python -B ai_module/main.py --parser rules "change the background to blue"
```

## Important LLM code

- `SCHEMA` defines the three allowed command shapes. All fields are required
  and extra properties are forbidden. An internal `result` wrapper allows a
  nested choice of shapes because the schema root must be an object.
  `result: null` represents unsupported input, not a fourth command.
- `parse_instruction` reads the environment, sends the request to the Responses
  API using Python's standard library, then calls `_decode_response`. No tools
  are supplied to the model.
- `_decode_response` rejects incomplete responses, refusals, malformed JSON,
  duplicate keys, invalid wrappers, and multiple text outputs. A null result
  raises ValueError. The wrapper is never returned to the caller.
- `validate_command` independently checks exact keys and types. RGB requires
  three numbers in [0, 1], excluding booleans, infinity, and NaN. Wireframe
  requires a boolean. Reset permits no extra fields.

The prompt asks the model to reject ambiguous, negated, unsupported, and
multiple-action requests. For example, "toggle wireframe" is ambiguous because
the module has no scene state. Validation guarantees the command's structure,
not correct interpretation of natural language.

Invalid/unsupported output raises ValueError. Configuration, HTTP, and network
failures raise RuntimeError. The CLI prints a concise error on stderr and exits
with status 1. It never prints the API key or raw server error bodies.

API reference: [OpenAI Structured Outputs documentation](https://developers.openai.com/api/docs/guides/structured-outputs).

## Local Ollama backend

With Ollama already running and `qwen3.5:4b` installed, run from the project root:

```powershell
python -B ai_module/main.py --parser ollama "make the background feel like midnight"
python -B ai_module/main.py --parser ollama "I want to see the edges of every triangle"
python -B ai_module/main.py --parser ollama "go back to normal solid rendering"
```

Expected interpretations: a dark background color (exact RGB is model-chosen),
`set_wireframe` with `enabled: true`, and `set_wireframe` with `enabled: false`.
The module does not start Ollama, install models, or execute returned commands.

Defaults are `http://localhost:11434` and `qwen3.5:4b`. Optional overrides:

```powershell
$env:OLLAMA_HOST = 'http://localhost:11434'
$env:OLLAMA_MODEL = 'qwen3.5:4b'
```

Use a full HTTP URL for the host. Requests go to the configured host; leave it
at the local default to keep inference on this machine. No OpenAI key is needed.
No extra Python packages are required. `rules` remains the default backend.

`parse_instruction(text)` sends one non-streaming POST to `/api/chat`, providing
the JSON schema in `format` and in the prompt. Temperature is zero, thinking is
disabled, and output is capped at 256 tokens. The timeout is 120 seconds to allow
model loading. There is no automatic retry or fallback to another parser.

`_decode_response` requires a completed normal stop, an assistant message with
no tool calls, and one JSON wrapper containing only `result`. It rejects broken
JSON, duplicate keys, non-finite constants, and null/unsupported results. It then
calls the existing `validate_command` for exact command fields, types, and RGB
bounds. Neither existing parser file is modified; importing these helpers has
no network side effects. Malformed/unsupported output raises `ValueError`;
server/configuration failures raise `RuntimeError`, reported by the CLI.

Schema validation checks structure, not interpretation accuracy. Color moods
are approximate. No current scene state is available, so relative toggles are
ambiguous. The backend never strips code fences or repairs malformed output.

References: [Ollama chat API](https://docs.ollama.com/api/chat) and
[structured outputs](https://docs.ollama.com/capabilities/structured-outputs).

## Optional renderer file bridge

Without `--bridge`, all parsers keep their existing JSON-only behavior. With it:

```powershell
python -B ai_module/main.py --bridge "change the background to blue"
python -B ai_module/main.py --parser ollama --bridge "give me a cold dark blue background"
```

The flag also works with `--parser llm`. The bridge reuses `validate_command`
without calling OpenAI. Only `set_background` is supported. For example, RGB
`[0.1, 0.2, 0.4]` is written as this single line, followed by a newline:

```text
set_background 0.1 0.2 0.4
```

The fixed destination is `C:\Users\Wang\Desktop\project\ai_module\renderer_command.txt`,
resolved relative to `bridge.py`, not the terminal's working directory. A unique
temporary file is written, flushed, and closed in the same directory before
`os.replace` publishes it atomically. Cleanup removes leftover temporary files
on failure. A replacement failure is reported, not retried with a partial write.

An unconsumed command is replaced: this is a single-command mailbox, not a queue.
The renderer owns consumption and deletion. Successful publication is not an
acknowledgement that the renderer has applied the command.

Successful bridge mode still prints command JSON to stdout and reports the
destination on stderr. Invalid commands or valid but unsupported commands
(`set_wireframe`, `reset_scene`) produce an error and exit status 1 without
touching the mailbox. There is no guessing or conversion to another command.

## Graphical control window

From the project root, launch:

```powershell
python -B ai_module/gui.py
```

Ollama must already be running with the configured model installed. The existing
default is `qwen3.5:4b`; `OLLAMA_HOST` and `OLLAMA_MODEL` overrides still apply.
The GUI uses Python's built-in Tkinter and needs no extra Python packages.

Type an instruction such as "give me a cold dark blue background" and click
**Send** or press **Enter**. The read-only output area shows your instruction,
the interpreted JSON, and the bridge result. Every successfully parsed command
is passed to the existing bridge. Unsupported instructions show a parser error;
wireframe/reset commands show their JSON followed by the bridge's unsupported
command error. Only background commands can currently reach the mailbox.

`ControlWindow.send` starts a background thread and disables input/Send until
the request finishes. `process` calls the existing `parse_instruction` and
`write_command` functions without duplicating their logic. A queue carries
results to `poll_events`, which updates Tkinter widgets on the main thread.
This keeps the window responsive while Ollama loads or generates a response.

"Sent to bridge" means the file was published, not that the renderer applied
it. Closing the window ends the application; it cannot recall a command already
published. The terminal CLI, parser implementations, and bridge are unchanged.
