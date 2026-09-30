# Standalone instruction parser

This prototype converts English instructions into structured command data using
rules (default), OpenAI, or local Ollama. By default it prints JSON only. Optional
`--bridge` publishes a background, object-color, or wireframe command for the renderer through
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
- `bridge.py`: validates commands and atomically publishes color/wireframe
  commands to `renderer_command.txt` beside this file.
- `llm_parser.py`: sends an OpenAI API request with a strict schema and validates
  the response locally before returning a command.
- `ollama_parser.py`: calls the local Ollama server and returns the same validated
  commands. Reuses schema/validation helpers without calling OpenAI.
- `test_wireframe.py`: offline regression checks using temporary mailboxes and
  mocked model responses; also checks the actual C++ reader when g++ is available.
- `test_object_color.py`: object-color parsing, validation, CLI/GUI, and combined
  renderer-state checks using temporary mailboxes and mocked model responses.

## Command format

Each successful call returns one Python dictionary. The command-line interface
serializes that dictionary as a JSON object.

| Command | Fields | Meaning / bridge support |
| --- | --- | --- |
| `set_background` | `color`: three RGB numbers from 0.0 to 1.0 | Set background color |
| `set_object_color` | `r`, `g`, `b`: each a finite number from 0.0 to 1.0 | Color the original yellow triangle |
| `set_wireframe` | `enabled`: boolean | Explicitly enable or disable wireframe |
| `reset_scene` | No additional fields | Parser-only; bridge does not support reset yet |

Examples:

```json
{"command": "set_background", "color": [0.0, 0.0, 1.0]}
{"command": "set_object_color", "r": 1.0, "g": 0.0, "b": 0.0}
{"command": "set_wireframe", "enabled": true}
{"command": "set_wireframe", "enabled": false}
{"command": "reset_scene"}
```

These are five separate command examples, not one JSON document. Reset behavior
is not implemented here; a future renderer consumer must define its defaults.
The existing background command keeps its `color` array for compatibility;
only the new object-color command uses separate `r`, `g`, `b` fields. The mailbox
is still plain text, not JSON; the bridge performs the existing conversion.

## How rule-based parsing works

1. Convert text to lowercase, collapse repeated whitespace, and remove trailing
   sentence punctuation (`.`, `!`, or `?`).
2. Match the entire instruction against a small set of supported patterns.
3. Return a command dictionary, or raise `ValueError` if nothing matches.

Supported phrases:

- `change the background to blue` or `set background to blue`: replace `blue`
  with `black`, `white`, `red`, or `green` as well. `the` is optional.
- Wireframe on: `switch to wireframe mode`, `enable wireframe`,
  `enable wireframe mode`, `turn on wireframe`, `turn wireframe on`,
  `wireframe on`, `show wireframe`, or `use wireframe mode`.
- Wireframe off: `switch to fill mode`, `disable wireframe`,
  `disable wireframe mode`, `turn off wireframe`, `turn wireframe off`,
  `wireframe off`, `solid mode`, or `go back to solid rendering`.
- Chinese on: `打开线框`, `打开线框模式`, `开启线框模式`.
  Chinese off: `关闭线框`, `关闭线框模式`.
- `reset scene` or `reset the scene`.
- Object colors: `make the object red`, `make the triangle blue`, `make it orange`,
  `change the object to green`, `change the object color to green`,
  `color the object purple`, or `set object color to orange`.
- Chinese object colors: `把物体改成红色`, `把物体颜色改成蓝色`,
  `把三角形变成绿色`, or `把物体设为紫色`.

Object color names supported by rules: red, green, blue, yellow, orange, purple,
white, black, gray/grey, cyan, and magenta. Light/dark blue, green, and red are
also supported. Chinese equivalents include 红色、绿色、蓝色、黄色、橙色、紫色、白色、
黑色、灰色、青色、品红色 and 浅/深蓝色、浅/深绿色、浅/深红色. Rules use a fixed RGB
table: orange is `(1, 0.5, 0)`, purple is `(0.5, 0, 0.5)`, light colors mix the
base with 50% white, and dark colors use half the base RGB. Ollama can interpret
other descriptions, such as bright red, without promising an exact shade.

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

These print the corresponding JSON commands. `-B` prevents Python from writing
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

All backends preserve this function signature and command format. The default
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

- `SCHEMA` defines four allowed parser command shapes. All fields are required
  and extra properties are forbidden. An internal `result` wrapper allows a
  nested choice of shapes because the schema root must be an object.
  `result: null` represents unsupported input, not an additional command.
- `parse_instruction` reads the environment, sends the request to the Responses
  API using Python's standard library, then calls `_decode_response`. No tools
  are supplied to the model.
- `_decode_response` rejects incomplete responses, refusals, malformed JSON,
  duplicate keys, invalid wrappers, and multiple text outputs. A null result
  raises ValueError. The wrapper is never returned to the caller.
- `validate_command` independently checks exact keys and types. RGB requires
  three numbers in [0, 1], excluding booleans, infinity, and NaN. Object RGB
  uses the same numeric constraints and requires exactly `command`, `r`, `g`, `b`. Wireframe
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
bounds. Importing these helpers has no network side effects.
Malformed/unsupported output raises `ValueError`;
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
without calling OpenAI. `set_background`, `set_object_color`, and `set_wireframe`
are supported. For example, background RGB
`[0.1, 0.2, 0.4]` is written as this single line, followed by a newline:

```text
set_background 0.1 0.2 0.4
```

Wireframe JSON uses a real boolean (`true` or `false`). The existing plain-text
mailbox transports it as `set_wireframe true` or `set_wireframe false`, followed
by a newline. The renderer accepts only these exact lowercase boolean tokens.
Object-color JSON is written as `set_object_color R G B` followed by a newline,
for example `set_object_color 1.0 0.5 0.0` for orange. Numeric strings, booleans,
missing/extra fields, non-finite values, and out-of-range RGB are rejected before
any file write. The renderer also validates the new command before changing state.

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
(`reset_scene`) produce an error and exit status 1 without
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
background/object-color/wireframe commands show their JSON and bridge status. Reset commands
still show the bridge's unsupported-command error.

`ControlWindow.send` starts a background thread and disables input/Send until
the request finishes. `process` calls the existing `parse_instruction` and
`write_command` functions without duplicating their logic. A queue carries
results to `poll_events`, which updates Tkinter widgets on the main thread.
This keeps the window responsive while Ollama loads or generates a response.

"Sent to bridge" means the file was published, not that the renderer applied
it. Closing the window ends the application; it cannot recall a command already
published. The CLI still defaults to rules and prints JSON only unless `--bridge`
is requested.

## Natural-language wireframe milestone

Supported AI renderer controls are background color, object color, and wireframe on/off:

```json
{"command":"set_background","color":[0.1,0.2,0.4]}
{"command":"set_object_color","r":1.0,"g":0.0,"b":0.0}
{"command":"set_wireframe","enabled":true}
{"command":"set_wireframe","enabled":false}
```

Try these in the GUI (Ollama), or pass them to `main.py --parser ollama --bridge`:

```text
make the background dark blue
make the object red
make the object purple
turn on wireframe
turn off wireframe
go back to solid rendering
打开线框模式
关闭线框模式
```

The C++ mailbox reader now updates the existing `wireframe` boolean used by W/E.
It runs before keyboard handling, so a held W/E key wins within that frame.
The original drawing behavior is preserved: most geometry switches between
line/fill, while tip-to-tip geometry keeps its filled base plus a wireframe
overlay. The renderer's build configuration is unchanged. The later object-color
milestone changes only the formerly constant-yellow fragment shader (see below).

Rebuild the renderer with the existing Visual Studio CMake setup to include the
new reader. From a Visual Studio developer PowerShell with the C++ tools loaded:

```powershell
cmake --build C:\Users\Wang\Desktop\project\CMakeProject1\out\build\x64-debug
```

Launch the rebuilt renderer from its existing build directory so its relative
texture path still resolves:

```powershell
Set-Location C:\Users\Wang\Desktop\project\CMakeProject1\out\build\x64-debug
.\hello.exe
```

In a second terminal, launch the GUI (with local Ollama already running):

```powershell
python -B C:\Users\Wang\Desktop\project\ai_module\gui.py
```

Offline checks from the project root:

```powershell
python -B -m unittest discover -s ai_module -p "test_*.py" -v
```

These check English/Chinese aliases, background regression, invalid booleans,
atomic mailbox output/cleanup, mocked model requests, CLI routing, and mocked GUI
construction/worker flow. If g++ is available, they extract and compile the actual
mailbox reader in isolation and check consumption/state changes. All test files
stay under a temporary directory in `ai_module`; the real mailbox is untouched.
GUI construction is mocked, not a visual test. Visual rendering, W/E interaction,
and the live GUI-to-renderer path still require checking with the rebuilt renderer.

## Persistent object color

The controlled object is the original solid yellow triangle (`vertices2`,
`vao2`, `shader_program_yellow`), just right of center and mostly below the
horizontal middle of the window. "Object", "triangle", and "it" all refer to
this one object; this milestone does not add object selection.

Its fragment shader originally returned constant yellow. It now uses
`uniform vec3 objectColor`, initialized from persistent C++ state `{1, 1, 0}`.
The mailbox reader accepts `set_object_color R G B`, validates all three values,
and changes only that state. Immediately before drawing `vao2`, C++ selects its
shader and uploads RGB with `glUniform3f`. The shader outputs
`vec4(objectColor, 1.0)`, so both filled fragments and wireframe edges use the
requested color. The per-vertex color example, C-key tint animation, texture,
other triangles, and existing wireframe drawing remain intact.

Object color is initialized once, outside the frame loop, and persists until
another object-color command or application restart. Background and wireframe
commands do not reset it. Send these as three separate instructions, allowing
the renderer to consume each mailbox file before sending the next:

```text
make the background dark blue
make the object orange
turn on wireframe
```

The resulting state is a dark blue background, an orange controlled triangle,
and wireframe enabled. Sending `make the object blue` then changes only that
triangle's color. This is a state guarantee tested through the mailbox reader,
not a claim of visual verification. The mailbox remains a single replaceable
command, so rapidly publishing commands before consumption can replace a pending
one; it is not a queue or multi-command plan.

CLI examples from the project root:

```powershell
python -B ai_module/main.py "make the object red"
python -B ai_module/main.py --bridge "make it orange"
python -B ai_module/main.py --bridge "把物体改成红色"
python -B ai_module/main.py --bridge "把三角形变成蓝色"
python -B ai_module/main.py --parser ollama --bridge "make the object bright red"
```

The first command only prints JSON; `--bridge` publishes to the renderer. The GUI
publishes automatically. Rebuild/run instructions above apply to this milestone
as well. Tests never send commands to the real renderer mailbox.
