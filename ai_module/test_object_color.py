"""Offline object-color checks; every mailbox lives in a temporary test folder."""

import contextlib
import io
import json
import os
from pathlib import Path
import queue
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import bridge
import commands
import gui
import llm_parser
import main
import ollama_parser


MODULE_DIR = Path(__file__).resolve().parent


def object_command(r, g, b):
    return {"command": "set_object_color", "r": r, "g": g, "b": b}


def ollama_response(content):
    return {"done": True, "done_reason": "stop", "message": {
        "role": "assistant", "content": content}}


class ObjectColorTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix=".object-color-test-", dir=str(MODULE_DIR))
        self.addCleanup(self.folder.cleanup)
        self.mailbox = Path(self.folder.name) / "test-command.txt"
        self.path_patch = patch.object(bridge, "BRIDGE_PATH", self.mailbox)
        self.path_patch.start()
        self.addCleanup(self.path_patch.stop)

    def test_rule_examples_and_colors(self):
        examples = {
            "make the object red": (1.0, 0.0, 0.0),
            "make the triangle blue": (0.0, 0.0, 1.0),
            "change the object to green": (0.0, 1.0, 0.0),
            "change the object color to green": (0.0, 1.0, 0.0),
            "make it orange": (1.0, 0.5, 0.0),
            "color the object purple": (0.5, 0.0, 0.5),
            "set object color to orange": (1.0, 0.5, 0.0),
            "make the object light blue": (0.5, 0.5, 1.0),
            "make the object dark blue": (0.0, 0.0, 0.5),
            "make the object light green": (0.5, 1.0, 0.5),
            "make the object dark green": (0.0, 0.5, 0.0),
            "make the object light red": (1.0, 0.5, 0.5),
            "make the object dark red": (0.5, 0.0, 0.0),
            "把物体改成红色": (1.0, 0.0, 0.0),
            "把三角形变成蓝色": (0.0, 0.0, 1.0),
            "把物体颜色改成绿色": (0.0, 1.0, 0.0),
            "把物体颜色改成蓝色": (0.0, 0.0, 1.0),
            "把三角形变成绿色": (0.0, 1.0, 0.0),
            "把物体设为紫色": (0.5, 0.0, 0.5),
            "  MAKE THE OBJECT   RED!  ": (1.0, 0.0, 0.0),
        }
        for color, rgb in {
            "red": (1.0, 0.0, 0.0), "green": (0.0, 1.0, 0.0),
            "blue": (0.0, 0.0, 1.0), "yellow": (1.0, 1.0, 0.0),
            "orange": (1.0, 0.5, 0.0), "purple": (0.5, 0.0, 0.5),
            "white": (1.0, 1.0, 1.0), "black": (0.0, 0.0, 0.0),
            "gray": (0.5, 0.5, 0.5), "grey": (0.5, 0.5, 0.5),
            "cyan": (0.0, 1.0, 1.0), "magenta": (1.0, 0.0, 1.0),
        }.items():
            examples["make the object " + color] = rgb
        for phrase, rgb in examples.items():
            with self.subTest(phrase=phrase):
                command = commands.parse_instruction(phrase)
                self.assertEqual(command, object_command(*rgb))
                self.assertTrue(all(type(command[channel]) is float for channel in ("r", "g", "b")))
        for phrase in ("make the object invisible", "do not make the object red",
                       "make the object red and turn on wireframe", "move the object red"):
            with self.subTest(phrase=phrase), self.assertRaises(ValueError):
                commands.parse_instruction(phrase)

    def test_bridge_valid_colors_and_boundaries(self):
        for rgb, expected in (
            ((0, 0, 0), b"set_object_color 0.0 0.0 0.0\n"),
            ((1, 1, 1), b"set_object_color 1.0 1.0 1.0\n"),
            ((1.0, 0.5, 0.0), b"set_object_color 1.0 0.5 0.0\n"),
        ):
            with self.subTest(rgb=rgb):
                command = object_command(*rgb)
                validated = llm_parser.validate_command(command)
                self.assertTrue(all(type(validated[channel]) is float for channel in ("r", "g", "b")))
                self.assertEqual(bridge.write_command(command), self.mailbox)
                self.assertEqual(self.mailbox.read_bytes(), expected)
                self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_bridge_rejects_invalid_colors_without_replacing_pending_command(self):
        self.mailbox.write_text("pending command")
        invalid = []
        for channel in ("r", "g", "b"):
            for value in (-0.01, 1.01, "red", True, False, None, [], {}, float("nan"),
                          float("inf"), -float("inf")):
                command = object_command(0.5, 0.5, 0.5)
                command[channel] = value
                invalid.append(command)
            command = object_command(0.5, 0.5, 0.5)
            del command[channel]
            invalid.append(command)
        invalid += [
            {"command": "set_object_color", "color": [1, 0, 0]},
            dict(object_command(1, 0, 0), extra=True),
            {"command": "execute", "code": "anything"},
            [object_command(1, 0, 0)],
        ]
        for command in invalid:
            with self.subTest(command=command), self.assertRaises(ValueError):
                bridge.write_command(command)
            self.assertEqual(self.mailbox.read_text(), "pending command")
        self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_atomic_replace_failure_preserves_pending_command_and_cleans_temp(self):
        self.mailbox.write_text("pending command")
        with patch.object(bridge.os, "replace", side_effect=PermissionError("locked")):
            with self.assertRaises(RuntimeError):
                bridge.write_command(object_command(1.0, 0.5, 0.0))
        self.assertEqual(self.mailbox.read_text(), "pending command")
        self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_mocked_ollama_structured_response(self):
        # Verify real request/decoding code without contacting any model server.
        command = object_command(0.05, 0.1, 0.35)
        response = ollama_response(json.dumps({"result": command}))
        with patch.dict(os.environ, {}, clear=True), patch.object(
                ollama_parser, "urlopen", return_value=io.BytesIO(json.dumps(response).encode())) as send:
            self.assertEqual(ollama_parser.parse_instruction("make the object dark blue"), command)
        payload = json.loads(send.call_args[0][0].data)
        self.assertEqual(payload["model"], "qwen3.5:4b")
        self.assertEqual(payload["options"]["temperature"], 0)
        self.assertEqual(payload["format"], llm_parser.SCHEMA)
        branches = payload["format"]["properties"]["result"]["anyOf"]
        object_schema = next(branch for branch in branches if
                             branch.get("properties", {}).get("command", {}).get("enum") ==
                             ["set_object_color"])
        self.assertEqual(set(object_schema["required"]), {"command", "r", "g", "b"})
        self.assertIs(object_schema["additionalProperties"], False)
        for channel in ("r", "g", "b"):
            self.assertEqual(object_schema["properties"][channel]["type"], "number")
            self.assertEqual(object_schema["properties"][channel]["minimum"], 0)
            self.assertEqual(object_schema["properties"][channel]["maximum"], 1)

    def test_model_decoders_reject_malformed_or_unsupported_object_commands(self):
        invalid = (
            '{"result":{"command":"set_object_color","r":2,"g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":true,"g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":"red","g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":0,"g":0}}',
            '{"result":{"command":"set_object_color","r":NaN,"g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":Infinity,"g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":0,"r":1,"g":0,"b":0}}',
            '{"result":{"command":"set_object_color","r":0,"g":0,"b":0,"extra":0}}',
            '{"result":{"command":"rotate_object","angle":90}}',
            '{"result":null}',
            '[{"command":"set_object_color","r":1,"g":0,"b":0}]',
            'print("set_object_color")',
        )
        for content in invalid:
            openai_response = {"status": "completed", "output": [{"type": "message", "content": [
                {"type": "output_text", "text": content}]}]}
            with self.subTest(backend="ollama", content=content), self.assertRaises(ValueError):
                ollama_parser._decode_response(ollama_response(content))
            with self.subTest(backend="llm", content=content), self.assertRaises(ValueError):
                llm_parser._decode_response(openai_response)
        command = object_command(1.0, 0.0, 0.0)
        openai_response["output"][0]["content"][0]["text"] = json.dumps({"result": command})
        self.assertEqual(llm_parser._decode_response(openai_response), command)

    def test_cli_default_rules_prints_json_without_writing_mailbox(self):
        # Run the documented no-bridge invocation, with bytecode writing disabled.
        result = subprocess.run([sys.executable, "-B", str(MODULE_DIR / "main.py"),
                                 "make the object red"], capture_output=True, text=True, check=True,
                                cwd=str(MODULE_DIR))
        self.assertEqual(json.loads(result.stdout), object_command(1.0, 0.0, 0.0))
        self.assertEqual(result.stderr, "")
        with patch.object(sys, "argv", ["main.py", "make the object red"]), patch.object(
                bridge, "write_command") as writer, contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(main.main(), 0)
            self.assertEqual(json.loads(output.getvalue()), object_command(1.0, 0.0, 0.0))
            writer.assert_not_called()
        self.assertFalse(self.mailbox.exists())

    def test_cli_routes_all_backends_through_existing_bridge(self):
        command = object_command(1.0, 0.0, 0.0)
        for backend, module in (("rules", main), ("ollama", ollama_parser), ("llm", llm_parser)):
            with self.subTest(backend=backend), patch.object(
                    module, "parse_instruction", return_value=command), patch.object(
                    sys, "argv", ["main.py", "--parser", backend, "--bridge", "make the object red"]), \
                    contextlib.redirect_stdout(io.StringIO()) as output, \
                    contextlib.redirect_stderr(io.StringIO()) as errors:
                self.assertEqual(main.main(), 0)
                self.assertEqual(json.loads(output.getvalue()), command)
                self.assertIn("Wrote bridge command", errors.getvalue())
                self.assertEqual(self.mailbox.read_bytes(), b"set_object_color 1.0 0.0 0.0\n")

    def test_gui_worker_displays_json_and_bridge_success_or_error(self):
        app = gui.ControlWindow.__new__(gui.ControlWindow)
        app.events = queue.Queue()
        command = object_command(0.5, 0.0, 0.5)
        with patch.object(gui, "parse_instruction", return_value=command):
            app.process("color the object purple")
        kind, text = app.events.get_nowait()
        self.assertEqual(kind, "command")
        self.assertEqual(json.loads(text), command)
        self.assertEqual(app.events.get_nowait()[0], "success")
        self.assertEqual(app.events.get_nowait()[0], "done")
        self.assertEqual(self.mailbox.read_bytes(), b"set_object_color 0.5 0.0 0.5\n")
        with patch.object(gui, "parse_instruction", side_effect=ValueError("unsupported")):
            app.process("move the object")
        self.assertEqual(app.events.get_nowait(), ("error", "unsupported"))
        self.assertEqual(app.events.get_nowait()[0], "done")
        self.assertEqual(self.mailbox.read_bytes(), b"set_object_color 0.5 0.0 0.5\n")

    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable for isolated C++ reader check")
    def test_actual_cpp_reader_rejects_invalid_values_and_preserves_independent_states(self):
        # Extract the real reader. This executable contains no renderer/OpenGL code.
        source = (MODULE_DIR.parent / "CMakeProject1/CMakeProject1/main.cpp").read_text(encoding="utf-8")
        start = source.index("bool readAICommand(")
        end = source.index("\n}", start) + 2
        harness = '#include <filesystem>\n#include <fstream>\n#include <string>\n#include <cassert>\n#include <cmath>\n'
        harness += source[start:end]
        harness += r'''
int main() {
    const std::filesystem::path path = "test-command.txt";
    float r = 0, g = 0, b = 0;
    float objectColor[3] = {1, 1, 0};
    bool wireframe = false;
    auto read = [&]() { return readAICommand(path, r, g, b, wireframe, objectColor); };
    auto write = [&](const char* text) { std::ofstream file(path); file << text; };
    auto darkBackground = [&]() { return r == 0.05f && g == 0.1f && b == 0.35f; };
    auto orangeObject = [&]() { return objectColor[0] == 1 && objectColor[1] == 0.5f && objectColor[2] == 0; };
    assert(!read());
    write("set_background 0.05 0.1 0.35\n");
    assert(read() && darkBackground());
    assert(objectColor[0] == 1 && objectColor[1] == 1 && objectColor[2] == 0);
    write("set_object_color 1 0.5 0\n");
    assert(read() && darkBackground() && orangeObject() && !wireframe);
    assert(!std::filesystem::exists(path));
    write("set_wireframe true\n");
    assert(read() && darkBackground() && orangeObject() && wireframe);
    assert(!read() && darkBackground() && orangeObject() && wireframe);
    for (auto bad : {"set_object_color", "set_object_color 0.5 0.5",
                     "set_object_color -0.01 0.5 0.5", "set_object_color 2 0.5 0.5",
                     "set_object_color 0.5 -0.01 0.5", "set_object_color 0.5 2 0.5",
                     "set_object_color 0.5 0.5 -0.01", "set_object_color 0.5 0.5 2",
                     "set_object_color red 0 0", "set_object_color true 0 0",
                     "set_object_color nan 0 0", "set_object_color 0 inf 0",
                     "set_object_color 0 0 -inf", "set_object_color 1e999 0 0",
                     "set_object_color 1 0 0 extra",
                     "set_object_color 1 0 0\nset_wireframe false", "execute anything"}) {
        write(bad);
        assert(!read());
        assert(darkBackground() && orangeObject() && wireframe);
        assert(std::filesystem::exists(path));
    }
    write("set_object_color 0 0 1\n");
    assert(read() && darkBackground() && wireframe);
    assert(objectColor[0] == 0 && objectColor[1] == 0 && objectColor[2] == 1);
    write("set_wireframe false\n");
    assert(read() && darkBackground() && !wireframe);
    assert(objectColor[0] == 0 && objectColor[1] == 0 && objectColor[2] == 1);
    write("set_background 0 0 0\n");
    assert(read() && r == 0 && g == 0 && b == 0 && !wireframe);
    assert(objectColor[0] == 0 && objectColor[1] == 0 && objectColor[2] == 1);
    assert(!std::filesystem::exists(path));
}
'''
        cpp = Path(self.folder.name) / "object_reader_test.cpp"
        exe = Path(self.folder.name) / "object_reader_test.exe"
        cpp.write_text(harness, encoding="utf-8")
        subprocess.run([shutil.which("g++"), "-std=c++17", str(cpp), "-o", str(exe)],
                       check=True, cwd=self.folder.name)
        subprocess.run([str(exe)], cwd=self.folder.name, check=True)


if __name__ == "__main__":
    unittest.main()
