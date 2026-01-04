# convert.py
import re

with open('char.asm', 'r') as f:
    content = f.read()

# Find all hex values like $00 and change to 0x00
hex_values = re.findall(r'\$([0-9a-fA-F]{2})', content)

with open('charset_data.h', 'w') as f:
    f.write('const uint8_t settlers_charset[] = {\n    ')
    for i, val in enumerate(hex_values):
        f.write(f'0x{val}, ')
        if (i + 1) % 12 == 0:
            f.write('\n    ')
    f.write('\n};')

print("Conversion complete! Use #include \"charset_data.h\" in your main.cc")
