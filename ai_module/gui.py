"""Small Tkinter control window using the existing Ollama parser and bridge."""

import json
import queue
import threading
import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText

from ollama_parser import parse_instruction
from bridge import write_command


class ControlWindow:
    def __init__(self, root):
        self.root = root
        self.busy = False
        self.events = queue.Queue()
        root.title("Renderer AI Control")
        root.geometry("680x440")
        root.minsize(480, 320)

        panel = ttk.Frame(root, padding=16)
        panel.pack(fill="both", expand=True)
        ttk.Label(panel, text="Describe a background change:").pack(anchor="w")
        row = ttk.Frame(panel)
        row.pack(fill="x", pady=(8, 12))
        self.entry = ttk.Entry(row)
        self.entry.pack(side="left", fill="x", expand=True)
        self.entry.bind("<Return>", self.send)
        self.button = ttk.Button(row, text="Send", command=self.send)
        self.button.pack(side="right", padx=(8, 0))

        self.status = tk.StringVar(value="Ready — Ollama parser; background bridge only.")
        ttk.Label(panel, textvariable=self.status, wraplength=440).pack(anchor="w")
        self.output = ScrolledText(panel, wrap="word", height=14, state="disabled")
        self.output.pack(fill="both", expand=True, pady=(12, 0))
        self.entry.focus_set()
        # Only this main-thread polling callback touches widgets.
        root.after(100, self.poll_events)

    def append(self, text):
        self.output.configure(state="normal")
        self.output.insert("end", text + "\n\n")
        self.output.see("end")
        self.output.configure(state="disabled")

    def send(self, event=None):
        if self.busy:
            return "break"
        instruction = self.entry.get().strip()
        if not instruction:
            self.status.set("Enter an instruction first.")
            self.entry.focus_set()
            return "break"
        self.busy = True
        self.button.configure(state="disabled")
        self.entry.configure(state="disabled")
        self.status.set("Interpreting with Ollama…")
        self.append("You: " + instruction)
        threading.Thread(target=self.process, args=(instruction,), daemon=True).start()
        return "break"

    def process(self, instruction):
        """Worker thread: reuse existing logic, communicate through a queue."""
        try:
            command = parse_instruction(instruction)
            self.events.put(("command", json.dumps(command, indent=2)))
            destination = write_command(command)
            self.events.put(("success", "Sent to bridge: " + str(destination)))
        except Exception as error:
            # Surface failures instead of leaving the window stuck as busy.
            self.events.put(("error", str(error) or type(error).__name__))
        finally:
            self.events.put(("done", ""))

    def poll_events(self):
        try:
            while True:
                kind, text = self.events.get_nowait()
                if kind == "command":
                    self.append("Interpreted command:\n" + text)
                    self.status.set("Publishing to bridge…")
                elif kind == "success":
                    self.append(text)
                    self.status.set("Sent to bridge — renderer consumption is not confirmed.")
                elif kind == "error":
                    self.append("Error: " + text)
                    self.status.set("Error — see details below.")
                elif kind == "done":
                    self.busy = False
                    self.button.configure(state="normal")
                    self.entry.configure(state="normal")
                    self.entry.focus_set()
        except queue.Empty:
            pass
        self.root.after(100, self.poll_events)


def main():
    root = tk.Tk()
    ControlWindow(root)
    root.mainloop()


if __name__ == "__main__":
    main()
