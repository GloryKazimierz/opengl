"""Print command JSON, optionally publishing it to the file bridge."""

import argparse
import json
import sys

from commands import parse_instruction


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert one instruction into JSON; no renderer is contacted."
    )
    parser.add_argument("instruction", help="An instruction enclosed in quotes")
    parser.add_argument("--parser", choices=("rules", "llm", "ollama"), default="rules",
                        help="Parser backend (default: rules)")
    parser.add_argument("--bridge", action="store_true",
                        help="Also publish background commands to renderer_command.txt")
    args = parser.parse_args()

    try:
        parse = parse_instruction
        if args.parser == "llm":
            from llm_parser import parse_instruction as parse
        elif args.parser == "ollama":
            from ollama_parser import parse_instruction as parse
        command = parse(args.instruction)
        if args.bridge:
            from bridge import write_command
            destination = write_command(command)
    except (ValueError, RuntimeError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    print(json.dumps(command))
    if args.bridge:
        print(f"Wrote bridge command to {destination}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
