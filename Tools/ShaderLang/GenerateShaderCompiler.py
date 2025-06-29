import os
import re
import argparse

def replace_scanner(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
        
        # Replace.
        pattern = r"""
        (yylex\s*\(\s*)    # 函数名和左括号
        &\s*yylval\s*,\s*  # &yylval参数
        &\s*yylloc\s*,\s*  # &yylloc参数
        \b(scanner)\b      # 独立的scanner单词
        (\s*\)\s*;)        # 右括号和分号
        """
        new_content = re.sub(pattern, r'\1&yylval, &yylloc, YYLEX_PARAM\3', 
                            content, flags=re.VERBOSE | re.IGNORECASE)
        
        if new_content != content:
            with open(file_path, 'w', encoding='utf-8') as file:
                file.write(new_content)
            print(f"File updated : {file_path}.")

    except Exception as e:
        print(f"Error : {str(e)}")

def post_process(path):
    if os.path.isfile(path):
        replace_scanner(path)
    else:
        print(f"File not found : {path}.")

if __name__ == "__main__":
    generated_file_path = './Source/Runtime/ShaderLang/Generated/parser.tab.cpp'
    post_process(generated_file_path)
