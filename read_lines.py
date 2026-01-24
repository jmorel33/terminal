
import sys

def read_lines(filepath, start_line, num_lines):
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
            start_index = start_line - 1
            end_index = start_index + num_lines

            if start_index >= len(lines):
                print(f"Start line {start_line} is beyond file length.")
                return

            for i in range(start_index, min(end_index, len(lines))):
                print(f"{lines[i]}", end='')
    except FileNotFoundError:
        print(f"File {filepath} not found.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python read_lines.py <filepath> <start_line> <num_lines>")
    else:
        read_lines(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))
