#!/usr/bin/env python3
import argparse
import re
import os
import subprocess
from datetime import datetime

def print_colored(text, color_code='37', end='\n'):
    """Color print function using ANSI escape codes"""
    # ANSI escape code prefix
    prefix = '\033['
    # End escape code
    suffix = '\033[0m'
    # Color code mapping
    colors = {
        'red': '31',
        'green': '32',
        'yellow': '33',
        'blue': '34',
        'purple': '35',
        'cyan': '36',
        'white': '37',
    }
    # Use the provided color code or default color
    color = colors.get(color_code, color_code)
    print(f"{prefix}{color}m{text}{suffix}", end=end)

def modify_verilog_variable_label(verilog_file, variable_name, new_label):
    """
    Find the security label for a specified variable in a Verilog file
    and replace it with a new security label.
    Assumes security labels are defined inside curly braces before variable declarations,
    e.g.: reg {c} b; or reg {Domain x} x;
    """
    # Regex to match the label block (including braces) before the variable name
    pattern = r"\{[^{}]*\}(?=\s+" + re.escape(variable_name) + ")"

    # Try to read the first match to determine if there is a match and get the old label
    with open(verilog_file, 'r') as file:
        content = file.read()
        match = re.search(pattern, content)
        if match:
            old_label = match.group(0)  # Get the matched old label

            # If the new label differs from the old label, perform the replacement
            if old_label != f"{{{new_label}}}":
                modified_content = re.sub(pattern, f"{{{new_label}}}", content)
                with open(verilog_file, 'w') as file:
                    file.write(modified_content)
               # print_colored(f"Modified label for '{variable_name}' to {{{new_label}}} in '{verilog_file}'.",'green')
                print_colored(f" '{variable_name}' label is to {{{new_label}}}.",'green')
            else:
               # print_colored(f"'{variable_name}'Label is already set to {{{new_label}}}, no change needed in '{verilog_file}'.",'green')
                print_colored(f" '{variable_name}' label is to {{{new_label}}}.",'green')
        else:
            print_colored(f"No found variable '{variable_name}' in '{verilog_file}'.",'red')


def run_iverilog_and_check(iverilog_executable, verilog_file, top_module, backend_file):
    """Run DIFCVerilog and the fixed Prolog backend."""
    stem = top_module if top_module else os.path.splitext(os.path.basename(verilog_file))[0]
    command_iverilog = [iverilog_executable, "-z"]
    if top_module:
        command_iverilog.extend(["-s", top_module])
    command_iverilog.append(verilog_file)
    subprocess.run(command_iverilog, check=True)
    subprocess.run(["swipl", "-q", "-s", backend_file, "--", "--stem", stem], check=True)

def process_label_sets(config_file):
    """Process multiple sets of label modifications from a config file"""
    with open(config_file, 'r') as file:
        current_set = None
        labels_to_modify = {}
        for line in file:
            line = line.strip()
            if line.startswith('[SET'):
                # New modification set begins, process the previous set (if exists)
                if current_set is not None:
                    yield current_set, labels_to_modify
                    labels_to_modify = {}
                current_set = line
            elif '=' in line:  # Assume format: variable_name=label_value, regex matching may need adjustment
                variable, label = line.split('=')
                labels_to_modify[variable.strip()] = label.strip()
        # Process the last modification set
        if current_set is not None:
            yield current_set, labels_to_modify


def main():
    parser = argparse.ArgumentParser(description="Modify security labels for variables and run DIFCVerilog.")
    parser.add_argument("-o", "--output", required=True, help="The Verilog file to be modified.")
    parser.add_argument("-c","--config-file", required=True, help="File containing variable-name new-label pairs.")
    parser.add_argument("--iverilog-executable", default="iverilog", help="Path to the Iverilog executable.")
    parser.add_argument("-s", "--top-module", default=None, help="Optional top module.")
    parser.add_argument("--backend", default="DIFCVerilog/difc_backend.pl", help="Path to difc_backend.pl.")

    args = parser.parse_args()

    if not os.path.exists(args.output):
        print(f"The file {args.output} does not exist.")
        return

    if not os.path.exists(args.config_file):
        print(f"The config file {args.config_file} does not exist.")
        return

    for set_name, labels_to_modify in process_label_sets(args.config_file):
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"\nProcessing label set: {set_name} at {current_time}")

        for variable_name, new_label in labels_to_modify.items():
            modify_verilog_variable_label(args.output, variable_name, new_label)
           # print(f"Modified label for '{variable_name}' to '{new_label}' in '{args.output}' at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"All labels in set '{set_name}' have been modified. Running Iverilog simulation at {current_time}")
        run_iverilog_and_check(args.iverilog_executable, args.output, args.top_module, args.backend)


       # print(f"Iverilog simulation for set '{set_name}' completed at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

if __name__ == "__main__":
    main()
