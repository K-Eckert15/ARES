search = "Terminal"

with open("PHOBOS_Control_Digital.kicad_pcb", "r") as file:
    for line_num, line in enumerate(file,start=1):
        if search in line:
            print(f"Line {line_num}: {line.strip()}")
#            print(line)
