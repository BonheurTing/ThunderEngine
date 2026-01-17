#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
import time
import re

def process_solution_file(solution_path, output_path):
    try:
        with open(solution_path, 'rb') as f:
            content = f.read()

        modified_content = re.sub(b"\"([^\"]+).vcxproj\"", b"\"Intermediate/Build/\\1.vcxproj\"", content)
        
        with open(output_path, 'wb') as f:
            f.write(modified_content)

        print(f"Successfully processed {os.path.basename(solution_path)} -> {os.path.basename(output_path)}")
        return True

    except Exception as e:
        print(f"Error processing solution file: {e}")
        return False

if __name__ == "__main__":
    os.system(".\\Tools\\ShaderLang\\GenerateShaderCompiler.bat")
    os.system(".\\InitializeInternal.bat")
    time.sleep(5)

    solution_files = [
        "./Intermediate/Build/ThunderEditor.sln",
        "./Intermediate/Build/ThunderEditor.slnx"
    ]
    
    found_solution = False

    for solution_path in solution_files:
        if os.path.exists(solution_path):
            file_ext = os.path.splitext(solution_path)[1]
            output_path = f"./ThunderEditor{file_ext}"
            
            if process_solution_file(solution_path, output_path):
                found_solution = True
                print(f"Succeeded to generate {os.path.basename(output_path)} file.")
                break

    if not found_solution:
        print('No solution file found in "./Intermediate/Build/"')
        print('Expected: ThunderEditor.sln or ThunderEditor.slnx')
        exit(1)