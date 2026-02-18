# convert.py
import re

with open('map.asm', 'r') as f:
    content = f.read()

# Find the character data section (after *=$2000)
char_data_pattern = r'\*=\$2000\n((?:\s*BYTE\s+\$[0-9a-fA-F]{2}(?:,\s*\$[0-9a-fA-F]{2})*\n?)+)'
match = re.search(char_data_pattern, content, re.IGNORECASE | re.MULTILINE)

if match:
    char_section = match.group(1)
    # Extract all hex values
    hex_values = re.findall(r'\$([0-9a-fA-F]{2})', char_section)
    
    # Convert to integers
    charset = [int(val, 16) for val in hex_values]
    
    print(f"Found {len(charset)} bytes = {len(charset)//8} characters\n")
    
    # Generate the header file
    with open('charset_data.h', 'w') as f:
        f.write('#ifndef CHARSET_DATA_H\n')
        f.write('#define CHARSET_DATA_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write('const uint8_t settlers_charset[] = {\n')
        
        total_chars = len(charset) // 8
        for ch in range(total_chars):
            offset = ch * 8
            f.write(f'    // Character ${ch:02x}\n')
            f.write('    ')
            
            # Write the 8 bytes
            bytes_line = []
            for i in range(8):
                bytes_line.append(f'0x{charset[offset+i]:02x}')
            
            f.write(', '.join(bytes_line))
            
            # Add comma if not last character
            if ch < total_chars - 1:
                f.write(',')
            
            # Add binary comment
            f.write('  // ')
            binary_line = []
            for i in range(8):
                byte = charset[offset + i]
                binary = format(byte, '08b').replace('1', '#').replace('0', '.')
                binary_line.append(binary)
            f.write(' '.join(binary_line))
            
            f.write('\n')
        
        f.write('};\n\n')
        f.write('#endif // CHARSET_DATA_H\n')
    
    print(f"Success! Generated charset_data.h with {len(hex_values)} bytes.")
    
else:
    print("Error: Could not find character data section starting at *=$2000")