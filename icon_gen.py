import re

icons = {
    "ICO_CLEAR": [
        "  #    #    #   ",
        "   #   #   #    ",
        "     #####      ",
        "   #########    ",
        " ############   ",
        " #############  ",
        "##############  ",
        "##############  ",
        "##############  ",
        "##############  ",
        " #############  ",
        " ############   ",
        "   #########    ",
        "     #####      ",
        "   #   #   #    ",
        "  #    #    #   "
    ],
    "ICO_MOON": [
        "       #####    ",
        "     #########  ",
        "   ####    #### ",
        "  ###        ## ",
        "  ##            ",
        " ###            ",
        " ###            ",
        " ###            ",
        " ###            ",
        " ###            ",
        " ###            ",
        "  ##            ",
        "  ###        ## ",
        "   ####    #### ",
        "     #########  ",
        "       #####    "
    ],
    "ICO_PARTLY": [
        "     #   #      ",
        "       #        ",
        "     #####      ",
        "   #########    ",
        "  ###########   ",
        "  ###     ####  ",
        " ###       #### ",
        "###         ### ",
        "##           ## ",
        "##           ## ",
        "##           ## ",
        "###         ### ",
        " ####     ####  ",
        "  ###########   ",
        "    #######     ",
        "                "
    ],
    "ICO_CLOUD": [
        "                ",
        "                ",
        "      ####      ",
        "    ########    ",
        "   ##########   ",
        "  ####    ####  ",
        " ###        ### ",
        "###          ###",
        "##            ##",
        "##            ##",
        "##            ##",
        "##            ##",
        "###          ###",
        " ############## ",
        "  ############  ",
        "                "
    ],
    "ICO_RAIN": [
        "      ####      ",
        "    ########    ",
        "   ##########   ",
        " ###        ### ",
        "###          ###",
        "##            ##",
        "##            ##",
        "###          ###",
        " ############## ",
        "  ############  ",
        "                ",
        "   #   #   #    ",
        "  #   #   #     ",
        "  #   #   #     ",
        "                ",
        "                "
    ],
    "ICO_SNOW": [
        "      ####      ",
        "    ########    ",
        "   ##########   ",
        " ###        ### ",
        "###          ###",
        "##            ##",
        "##            ##",
        "###          ###",
        " ############## ",
        "  ############  ",
        "                ",
        "   #   #   #    ",
        "  ### ### ###   ",
        "   #   #   #    ",
        "                ",
        "                "
    ],
    "ICO_STORM": [
        "      ####      ",
        "    ########    ",
        "   ##########   ",
        " ###        ### ",
        "###          ###",
        "##            ##",
        "##            ##",
        "###          ###",
        " ############## ",
        "  ############  ",
        "       #        ",
        "      ##        ",
        "     ####       ",
        "       ##       ",
        "      ##        ",
        "                "
    ],
    "ICO_FOG": [
        "                ",
        "                ",
        "  ############  ",
        "  ############  ",
        "                ",
        " ############## ",
        " ############## ",
        "                ",
        " ############## ",
        " ############## ",
        "                ",
        "  ############  ",
        "  ############  ",
        "                ",
        "                ",
        "                "
    ]
}

def to_gyver(lines):
    # 16 columns, 2 pages (top 8 rows, bottom 8 rows)
    out = []
    for page in range(2):
        for col in range(16):
            b = 0
            for row in range(8):
                y = page * 8 + row
                if lines[y][col] != ' ':
                    b |= (1 << row)
            out.append(b)
    
    # format as hex string: 0x00, 0x00, ... (16 per line)
    hex_str = ""
    for i, b in enumerate(out):
        hex_str += f"0x{b:02X},"
        if (i + 1) % 16 == 0:
            hex_str += "\n    "
    return hex_str.strip()[:-1]  # remove trailing comma and newline

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/DisplayManager.cpp", "r") as f:
    code = f.read()

for name, lines in icons.items():
    arr_str = to_gyver(lines)
    
    # regex to replace the existing array
    pattern = r"static const uint8_t PROGMEM " + name + r"\[\] = \{[\s\S]*?\};"
    repl = f"static const uint8_t PROGMEM {name}[] = {{\n    {arr_str}\n}};"
    
    code = re.sub(pattern, repl, code)

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/DisplayManager.cpp", "w") as f:
    f.write(code)
    
print("Icons updated!")
