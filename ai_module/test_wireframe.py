"""Offline milestone checks; never write to the renderer's real mailbox."""

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
from unittest.mock import Mock, patch

import bridge
import commands
import gui
import llm_parser
import main
import ollama_parser


MODULE_DIR = Path(__file__).resolve().parent


class WireframeTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix=".wireframe-test-", dir=str(MODULE_DIR))
        self.addCleanup(self.folder.cleanup)
        self.mailbox = Path(self.folder.name) / "test-command.txt"
        self.path_patch = patch.object(bridge, "BRIDGE_PATH", self.mailbox)
        self.path_patch.start()
        self.addCleanup(self.path_patch.stop)

    def test_rule_aliases(self):
        enabled = ("turn on wireframe", "enable wireframe", "enable wireframe mode",
                   "wireframe on", "show wireframe", "use wireframe mode",
                   "turn wireframe on", "switch to wireframe mode",
                   "打开线框", "打开线框模式", "开启线框模式")
        disabled = ("turn off wireframe", "disable wireframe", "wireframe off",
                    "turn wireframe off", "disable wireframe mode", "solid mode",
                    "go back to solid rendering", "switch to fill mode",
                    "关闭线框", "关闭线框模式")
        for value, phrases in ((True, enabled), (False, disabled)):
            for phrase in phrases:
                with self.subTest(phrase=phrase):
                    self.assertEqual(commands.parse_instruction(phrase),
                                     {"command": "set_wireframe", "enabled": value})
        self.assertEqual(commands.parse_instruction("  TURN ON   WIREFRAME! ")["enabled"], True)
        for phrase in ("do not turn on wireframe", "toggle wireframe",
                       "turn on wireframe and reset scene"):
            with self.assertRaises(ValueError):
                commands.parse_instruction(phrase)

    def test_background_and_wireframe_bridge(self):
        bridge.write_command(commands.parse_instruction("change the background to blue"))
        self.assertEqual(self.mailbox.read_bytes(), b"set_background 0.0 0.0 1.0\n")
        for enabled in (True, False):
            bridge.write_command({"command": "set_wireframe", "enabled": enabled})
            self.assertEqual(self.mailbox.read_text(), "set_wireframe " + str(enabled).lower() + "\n")
        self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_invalid_commands_preserve_mailbox(self):
        self.mailbox.write_text("pending command")
        bad = [{"command": "set_wireframe", "enabled": v}
               for v in (1, 0, "true", "false", None, [], {})]
        bad += [{"command": "set_wireframe"},
                {"command": "set_wireframe", "enabled": True, "extra": 1},
                {"command": "execute", "code": "anything"}, {"command": "reset_scene"}]
        for command in bad:
            with self.subTest(command=command), self.assertRaises(ValueError):
                bridge.write_command(command)
            self.assertEqual(self.mailbox.read_text(), "pending command")
        self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_replace_failure_cleans_temporary_file(self):
        self.mailbox.write_text("pending command")
        with patch.object(bridge.os, "replace", side_effect=PermissionError("locked")):
            with self.assertRaises(RuntimeError):
                bridge.write_command({"command": "set_wireframe", "enabled": True})
        self.assertEqual(self.mailbox.read_text(), "pending command")
        self.assertEqual(list(self.mailbox.parent.iterdir()), [self.mailbox])

    def test_ollama_request_and_response(self):
        for enabled in (True, False):
            command = {"command": "set_wireframe", "enabled": enabled}
            response = {"done": True, "done_reason": "stop", "message": {
                "role": "assistant", "content": json.dumps({"result": command})}}
            with patch.dict(os.environ, {}, clear=True), patch.object(
                    ollama_parser, "urlopen", return_value=io.BytesIO(json.dumps(response).encode())) as send:
                self.assertEqual(ollama_parser.parse_instruction("wireframe request"), command)
                payload = json.loads(send.call_args[0][0].data)
                self.assertEqual(payload["model"], "qwen3.5:4b")
                self.assertEqual(payload["format"], llm_parser.SCHEMA)
                self.assertEqual(payload["options"]["temperature"], 0)
        response["message"]["content"] = '{"result":{"command":"set_wireframe","enabled":"true"}}'
        with self.assertRaises(ValueError):
            ollama_parser._decode_response(response)

    def test_cli_compatibility(self):
        for phrase in ("turn on wireframe", "turn off wireframe", "change the background to blue"):
            with patch.object(sys, "argv", ["main.py", phrase]), patch.object(
                    bridge, "write_command") as writer, contextlib.redirect_stdout(io.StringIO()) as output:
                self.assertEqual(main.main(), 0)
                self.assertEqual(json.loads(output.getvalue()), commands.parse_instruction(phrase))
                writer.assert_not_called()
        for backend, module in (("rules", commands), ("ollama", ollama_parser), ("llm", llm_parser)):
            command = {"command": "set_wireframe", "enabled": True}
            with patch.object(module, "parse_instruction", return_value=command), patch.object(
                    sys, "argv", ["main.py", "--parser", backend, "--bridge", "turn on wireframe"]), \
                    contextlib.redirect_stdout(io.StringIO()) as output, \
                    contextlib.redirect_stderr(io.StringIO()) as errors:
                self.assertEqual(main.main(), 0)
                self.assertEqual(json.loads(output.getvalue()), command)
                self.assertIn("Wrote bridge command", errors.getvalue())
                self.assertEqual(self.mailbox.read_text(), "set_wireframe true\n")

    def test_gui_worker(self):
        app = gui.ControlWindow.__new__(gui.ControlWindow)
        app.events = queue.Queue()
        command = {"command": "set_wireframe", "enabled": False}
        with patch.object(gui, "parse_instruction", return_value=command):
            app.process("turn off wireframe")
        self.assertEqual(app.events.get()[0], "command")
        self.assertEqual(app.events.get()[0], "success")
        self.assertEqual(app.events.get()[0], "done")
        self.assertEqual(self.mailbox.read_text(), "set_wireframe false\n")
        with patch.object(gui, "parse_instruction", side_effect=ValueError("unsupported")):
            app.process("invalid")
        self.assertEqual(app.events.get(), ("error", "unsupported"))
        self.assertEqual(app.events.get()[0], "done")

    def test_gui_mocked_launch(self):
        # Construct all widgets and run entry point without opening a real window.
        with patch.object(gui.tk, "Tk") as root, patch.object(gui.tk, "StringVar"), \
                patch.object(gui.ttk, "Frame"), patch.object(gui.ttk, "Label"), \
                patch.object(gui.ttk, "Entry") as entry, patch.object(gui.ttk, "Button"), \
                patch.object(gui, "ScrolledText"):
            gui.main()
            root.return_value.mainloop.assert_called_once()
            entry.return_value.bind.assert_called_once()
            self.assertEqual(entry.return_value.bind.call_args[0][0], "<Return>")

    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable for isolated C++ reader check")
    def test_actual_cpp_mailbox_reader(self):
        # Compile the actual reader alone, not a copy or the OpenGL renderer.
        source = (MODULE_DIR.parent / "CMakeProject1/CMakeProject1/main.cpp").read_text(encoding="utf-8")
        start = source.index("bool readAICommand(")
        end = source.index("\n}", start) + 2
        harness = '#include <filesystem>\n#include <fstream>\n#include <string>\n#include <cassert>\n#include <cmath>\n'
        harness += source[start:end]
        harness += r'''
int main() {
    const std::filesystem::path path = "test-command.txt";
    float r = 0, g = 0, b = 0;
    bool wireframe = false;
    float objectColor[3] = {1, 1, 0};
    assert(!readAICommand(path, r, g, b, wireframe, objectColor));
    auto write = [&](const char* text) { std::ofstream file(path); file << text; };
    write("set_background 0.1 0.2 0.4\n");
    assert(readAICommand(path, r, g, b, wireframe, objectColor));
    assert(r == 0.1f && g == 0.2f && b == 0.4f && !wireframe);
    assert(!std::filesystem::exists(path));
    write("set_wireframe true\n");
    assert(readAICommand(path, r, g, b, wireframe, objectColor) && wireframe);
    assert(!std::filesystem::exists(path));
    write("set_wireframe false\n");
    assert(readAICommand(path, r, g, b, wireframe, objectColor) && !wireframe);
    for (auto bad : {"set_wireframe", "set_wireframe 1", "set_wireframe TRUE",
                     "set_wireframe true extra", "set_wireframe true\nset_wireframe false",
                     "unknown true"}) {
        write(bad);
        assert(!readAICommand(path, r, g, b, wireframe, objectColor));
        assert(!wireframe && r == 0.1f && g == 0.2f && b == 0.4f);
        assert(std::filesystem::exists(path));
    }
}
'''
        cpp = Path(self.folder.name) / "reader_test.cpp"
        exe = Path(self.folder.name) / "reader_test.exe"
        cpp.write_text(harness, encoding="utf-8")
        subprocess.run([shutil.which("g++"), "-std=c++17", str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], cwd=self.folder.name, check=True)


if __name__ == "__main__":
    unittest.main()
