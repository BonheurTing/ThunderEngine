#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
import time
import re

if __name__ == "__main__":
    os.system(".\GenerateShaderCompiler.bat")
    os.system(".\InitializeInternal.bat")
    time.sleep(5)
    solution_file_name = ".\\Intermediate\\Build\\ThunderEditor.sln"
    if os.path.exists(solution_file_name):
        solution_content = ""
        with open("./Intermediate/Build/ThunderEditor.sln", 'rb') as solution_file:
            solution_content = solution_file.read()
            solution_content = re.sub(b"\"([^\"]+).vcxproj\"", b"\"Intermediate/Build/\\1.vcxproj\"", solution_content)
        with open("./ThunderEditor.sln", 'wb') as final_file:
            final_file.write(solution_content)
        print("Succeeded to generate solution file.")
    else:
        print('Path "{}" not exist.'.format(solution_file_name))
        exit(1)