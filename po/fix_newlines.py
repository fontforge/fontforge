import os
import sys

def process_file(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8') as outfile:
        lines = infile.readlines()
        i = 0
        
        while i < len(lines):
            msgid = lines[i]
            buf_msgid = []
            buf_msgstr = []
            if msgid.startswith("msgid"):
                while lines[i+1].startswith("\""):
                    buf_msgid.append(lines[i])
                    i += 1
                msgid = lines[i]
                msgid_has_nl = lines[i].endswith('\\n\"\n')

                # Read the corresponding msgstr line
                i += 1
                while i+1 < len(lines) and lines[i+1].startswith("\""):
                    buf_msgstr.append(lines[i])
                    i += 1
                msgstr = lines[i]
                msgstr_has_nl = lines[i].endswith('\\n\"\n') 

                msgstr = lines[i]
                msgstr_has_nl = lines[i].endswith('\\n\"\n')
                # Check if msgid ends with a newline
                if msgstr_has_nl and not msgid_has_nl:
                    msgstr = msgstr[:-4] + '\"\n'  # Remove the newline character from msgstr
                if not msgstr_has_nl and msgid_has_nl:
                    msgstr = msgstr[:-2] + '\\n\"\n'  # Add the newline character to msgstr
                
                # Write the modified lines to the output file
                outfile.write("".join(buf_msgid))
                outfile.write(msgid)
                outfile.write("".join(buf_msgstr))
                outfile.write(msgstr)
            else:
                outfile.write(msgid)
            outfile.flush()
            
            i += 1

# Example usage
input_file = sys.argv[1]  # Replace with your input file name
output_file = input_file+".1"  # Replace with your desired output file name

process_file(input_file, output_file)
os.rename(output_file, input_file)
